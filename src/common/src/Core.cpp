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
	, m_currentNumOfElements{ nullptr }
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
		Log("Detected dead owner of a mutex, resetting the buffer...");
		ResetData();
		pthread_mutex_consistent(m_mutex);
		// We are not going to unlock it here, allowing the calling thread to work on the data with
		// mutex still locked.
	}
}

void ShBuf::unlock()
{
	pthread_mutex_unlock(m_mutex);
}

std::span<DataBlock>::iterator ShBuf::begin() const
{
	return m_data.begin();
}

std::span<DataBlock>::iterator ShBuf::end() const
{
	auto tempIter = m_data.end();
	std::advance(tempIter, -(m_data.size() - *m_currentNumOfElements));

	return tempIter;
}

std::size_t ShBuf::GetSize() const
{
	return *m_currentNumOfElements;
}

void ShBuf::Insert(const DataBlock& block)
{
	if (IsFull())
	{
		return;
	}

	Log("Inserting seqnum: {} Data: {}", block.seqnum, block.payload);

	m_data[*m_currentNumOfElements] = block;
	++(*m_currentNumOfElements);
}

bool ShBuf::IsFull() const
{
	if (nullptr != m_currentNumOfElements)
	{
		return *m_currentNumOfElements >= m_data.size();
	}

	return true;
}

void ShBuf::ResetData()
{
	*m_currentNumOfElements = 0;
}

void ShBuf::InitMemory(const std::size_t numberOfBlocks)
{
	int fd{ 0 };
	fd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd < 0 && errno == EEXIST)
	{
		fd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
	}
	else if (fd >= 0)
	{
		Log("First initialization");
		m_isCreator = true;
	}
	else
	{
		throw std::runtime_error{ fmt::format(
			"Error during shm_open call, errno: {}", strerror(errno)) };
	}

	m_memorySize = sizeof(decltype(*m_mutex)) + sizeof(decltype(*m_currentNumOfElements))
		+ (sizeof(DataBlock) * numberOfBlocks);

	Log("Memory allocated: {}", m_memorySize);

	const auto truncRes{ ftruncate(fd, m_memorySize) };
	if (truncRes < 0)
	{
		throw std::runtime_error{ fmt::format(
			"Error during ftruncate call, errno: {}", strerror(errno)) };
	}

	m_mutex =
		static_cast<pthread_mutex_t*>(mmap(NULL, m_memorySize, PROT_WRITE, MAP_SHARED, fd, 0));
	close(fd);

	if (MAP_FAILED == m_mutex)
	{
		shm_unlink(details::SHARED_MEMORY_NAME);
		throw std::runtime_error{ fmt::format(
			"Mmap returned null, errno: {}", std::strerror(errno)) };
	}
}

void ShBuf::InitState(const std::size_t numberOfBlocks)
{
	m_currentNumOfElements = reinterpret_cast<decltype(m_currentNumOfElements)>(m_mutex + 1);
	m_data = std::span<DataBlock>{
		reinterpret_cast<DataBlock*>(m_currentNumOfElements + 1),
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
		// Resetting the size should be done once and only in the process that created the shared
		// memory, in other cases we should just use the value that is already there.
		*m_currentNumOfElements = 0;
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
