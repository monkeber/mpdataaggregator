#include <common/Buffer.h>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/format.h>

#include <stdexcept>
#include <atomic>

namespace
{
namespace details
{

static std::atomic<bool> ShouldExit{ false };

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
{
	int fd{ 0 };
	bool firstInitialization{ false };
	fd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd < 0 && errno == EEXIST)
	{
		fd = shm_open(details::SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
	}
	else if (fd >= 0)
	{
		firstInitialization = true;
	}
	else
	{
		throw std::runtime_error{ fmt::format("Error during shm_open call, errno: {}", errno) };
	}

	m_memorySize = sizeof(decltype(m_mutex)) + sizeof(decltype(m_currentNumOfElements))
		+ (DATA_CHUNK_SIZE * numberOfBlocks);

	ftruncate(fd, m_memorySize);

	m_mutex =
		static_cast<pthread_mutex_t*>(mmap(NULL, m_memorySize, PROT_WRITE, MAP_SHARED, fd, 0));
	close(fd);

	if (m_mutex == MAP_FAILED)
	{
		shm_unlink(details::SHARED_MEMORY_NAME);
		throw std::runtime_error{ fmt::format("Mmap returned null, errno: {}", errno) };
	}

	if (firstInitialization)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(m_mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	m_currentNumOfElements = reinterpret_cast<decltype(m_currentNumOfElements)>(m_mutex + 1);
	*m_currentNumOfElements = 0;

	m_data = std::span<DataBlock>{ reinterpret_cast<DataBlock*>(m_currentNumOfElements + 1),
								   numberOfBlocks };
}

ShBuf::~ShBuf()
{
	munmap(m_mutex, m_memorySize);
	shm_unlink(details::SHARED_MEMORY_NAME);
}

void ShBuf::lock()
{
	pthread_mutex_lock(m_mutex);
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

void ShBuf::Insert(const DataBlock& block)
{
	if (IsFull())
	{
		return;
	}

	m_data[*m_currentNumOfElements] = block;
	++(*m_currentNumOfElements);
}

bool ShBuf::IsFull() const
{
	if (m_currentNumOfElements != nullptr)
	{
		return *m_currentNumOfElements >= m_data.size();
	}

	return true;
}

void ShBuf::ResetData()
{
	*m_currentNumOfElements = 0;
}

MQueue::MQueue(const int additionalFlags, const bool shouldUnlink)
	: m_shouldUnlink{ shouldUnlink }
{
	mq_attr queueAttr;
	queueAttr.mq_flags = 0;
	queueAttr.mq_maxmsg = 1;
	queueAttr.mq_msgsize = 1;
	queueAttr.mq_curmsgs = 0;

	m_queueDesc = mq_open(details::MESSAGE_QUEUE_NAME, O_CREAT | additionalFlags, 0666, &queueAttr);
}

MQueue::~MQueue()
{
	mq_close(m_queueDesc);
}

void MQueue::ReceiveNotify()
{
	char msg[1];
	mq_receive(m_queueDesc, msg, 1, nullptr);
}

void MQueue::SendNotify()
{
	mq_send(m_queueDesc, "", 0, 0);
}

void initSignalHandlers()
{
	struct sigaction psa;
	psa.sa_handler = common::signalHandler;
	sigaction(SIGTERM, &psa, NULL);
	sigaction(SIGINT, &psa, NULL);
}

void signalHandler(int signo)
{
	switch (signo)
	{
	case SIGTERM:
	case SIGINT:
		details::ShouldExit = true;
		break;
	}
}

bool shouldExit()
{
	return details::ShouldExit.load();
}

}	 // namespace common
