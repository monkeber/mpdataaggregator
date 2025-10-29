#include <common/Core.h>
#include <common/Utils.h>

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
namespace details
{

constexpr auto MESSAGE_QUEUE_NAME{ "/mpdataaggregatorqueue" };
constexpr auto SHARED_MEMORY_NAME{ "/mpdataaggregator" };

}	 // namespace details
}	 // namespace

namespace common
{

ShBuf::ShBuf(const std::size_t numberOfBlocks)
	: m_mutex{ nullptr }
	, m_readIndex{ nullptr }
	, m_writeIndex{ nullptr }
	, m_data{}
	, m_memorySize{ 0 }
	, m_isCreator{ false }
{
	InitMemory(numberOfBlocks);
	InitState(numberOfBlocks);
}

ShBuf::~ShBuf()
{
	munmap(m_mutex, m_memorySize);
	if (m_isCreator)
	{
		shm_unlink(details::SHARED_MEMORY_NAME);
	}
}

void ShBuf::lock()
{
	const auto ret{ pthread_mutex_lock(m_mutex) };
	if (EOWNERDEAD == ret)
	{
		Log("Detected dead owner of a mutex, resetting the buffer, possible data loss...");
		pthread_mutex_consistent(m_mutex);
		*m_writeIndex = 0;
		*m_readIndex = 0;
		// We are not going to unlock it here, allowing the calling thread to work on the data with
		// mutex still locked.
	}
}

void ShBuf::unlock()
{
	pthread_mutex_unlock(m_mutex);
}

void ShBuf::Insert(const DataBlock& block)
{
	Log("Inserting seqnum: {} Data: {}", block.seqnum, block.payload);

	m_data[*m_writeIndex] = block;
	*m_writeIndex = (*m_writeIndex + 1) % m_data.size();

	if (*m_writeIndex == *m_readIndex)
	{
		*m_readIndex = (*m_readIndex + 1) % m_data.size();
	}
}

std::optional<DataBlock> ShBuf::Read()
{
	// The buffer is empty and there is nothing to read.
	if (*m_readIndex == *m_writeIndex)
	{
		return std::nullopt;
	}

	std::optional<DataBlock> result{ m_data[*m_readIndex] };

	*m_readIndex = (*m_readIndex + 1) % m_data.size();

	return result;
}

void ShBuf::InitMemory(const std::size_t numberOfBlocks)
{
	int memFd{ 0 };
	memFd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (memFd < 0 && errno == EEXIST)
	{
		memFd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
	}
	else if (memFd >= 0)
	{
		Log("First initialization");
		m_isCreator = true;
	}
	else
	{
		throw std::runtime_error{ fmt::format(
			"Error during shm_open call, errno: {}", strerror(errno)) };
	}

	// Just a reminder to myself, dereferencing a nullptr in sizeof statement is fine, since
	// dereferencing is not evaluated unless the type is variable length array.
	m_memorySize = sizeof(*m_mutex) + sizeof(*m_readIndex) + sizeof(*m_writeIndex)
		+ (sizeof(DataBlock) * numberOfBlocks);

	Log("Memory allocated: {}", m_memorySize);

	const auto truncateRes{ ftruncate(memFd, m_memorySize) };
	if (truncateRes < 0)
	{
		throw std::runtime_error{ fmt::format(
			"Error during ftruncate call, errno: {}", strerror(errno)) };
	}

	// Even though we PROT_WRITE may imply PROT_READ on some implementations, better to specify
	// PROT_READ explicitly.
	m_mutex = static_cast<pthread_mutex_t*>(
		mmap(NULL, m_memorySize, PROT_WRITE | PROT_READ, MAP_SHARED, memFd, 0));
	close(memFd);

	if (MAP_FAILED == m_mutex)
	{
		shm_unlink(details::SHARED_MEMORY_NAME);
		throw std::runtime_error{ fmt::format(
			"Mmap returned null, errno: {}", std::strerror(errno)) };
	}
}

void ShBuf::InitState(const std::size_t numberOfBlocks)
{
	// Possible place for improvement, maybe move these members into a standalone struct to avoid
	// calculating their addresses because it is error prone.
	m_readIndex = reinterpret_cast<decltype(m_readIndex)>(m_mutex + 1);
	m_writeIndex = reinterpret_cast<decltype(m_writeIndex)>(m_readIndex + 1);
	m_data = std::span<DataBlock>{
		reinterpret_cast<DataBlock*>(m_writeIndex + 1),
		numberOfBlocks,
	};

	if (m_isCreator)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
		pthread_mutex_init(m_mutex, &attr);
		pthread_mutexattr_destroy(&attr);
		// Resetting the indeces should be done only on initial initialization of the memory (the
		// process that created the buffer), in all the following processes we should just read the
		// existing values.
		*m_readIndex = 0;
		*m_writeIndex = 0;
	}
}

MQueue::MQueue(const int additionalFlags)
	: m_isCreator{ false }
{
	mq_attr queueAttr;
	queueAttr.mq_flags = 0;
	queueAttr.mq_maxmsg = 1;
	// In this implementation we don't send any useful data in the messages.
	queueAttr.mq_msgsize = 1;
	queueAttr.mq_curmsgs = 0;

	m_queueDesc =
		mq_open(details::MESSAGE_QUEUE_NAME, O_CREAT | O_EXCL | additionalFlags, 0666, &queueAttr);
	if (0 > m_queueDesc && errno == EEXIST)
	{
		m_queueDesc =
			mq_open(details::MESSAGE_QUEUE_NAME, O_CREAT | additionalFlags, 0666, &queueAttr);
	}
	else if (0 <= m_queueDesc)
	{
		m_isCreator = true;
	}
	else
	{
		throw std::runtime_error{ fmt::format(
			"Error during shm_open call, errno: {}", std::strerror(errno)) };
	}
}

MQueue::~MQueue()
{
	mq_close(m_queueDesc);
	if (m_isCreator)
	{
		mq_unlink(details::MESSAGE_QUEUE_NAME);
	}
}

void MQueue::ReceiveNotify()
{
	char msg[1];
	const auto ret{ mq_receive(m_queueDesc, msg, 1, nullptr) };
	if (-1 == ret)
	{
		Log("Error on receiving the message: {}", strerror(errno));
	}
}

void MQueue::SendNotify()
{
	mq_send(m_queueDesc, "", 0, 0);
}

}	 // namespace common
