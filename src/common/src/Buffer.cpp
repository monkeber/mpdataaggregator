#include <common/Buffer.h>

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <fmt/format.h>

#include <stdexcept>

namespace common
{

ShBuf::ShBuf(const std::size_t numberOfBlocks)
	: m_mutex{ nullptr }
	, m_data{}
{
	int fd{ 0 };
	bool firstInitialization{ false };
	fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd < 0 && errno == EEXIST)
	{
		fd = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666);
	}
	else if (fd >= 0)
	{
		firstInitialization = true;
	}
	else
	{
		throw std::runtime_error{ fmt::format("Error during shm_open call, errno: {}", errno) };
	}

	ftruncate(fd, SHARED_MEMORY_LENGTH);

	m_mutex = reinterpret_cast<pthread_mutex_t*>(
		mmap(NULL, SHARED_MEMORY_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0));
	close(fd);

	if (m_mutex == MAP_FAILED)
	{
		shm_unlink(SHARED_MEMORY_NAME);
		throw std::runtime_error{ fmt::format("Mmap returned null, errno: {}", errno) };
	}

	m_data = std::span<DataBlock>{ reinterpret_cast<DataBlock*>(m_mutex + 1), numberOfBlocks };

	if (firstInitialization)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(m_mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	}
}

ShBuf::~ShBuf()
{
	munmap(m_mutex, SHARED_MEMORY_LENGTH);
	shm_unlink(SHARED_MEMORY_NAME);
}

void ShBuf::lock()
{
	pthread_mutex_lock(m_mutex);
}

void ShBuf::unlock()
{
	pthread_mutex_unlock(m_mutex);
}

DataBlock& ShBuf::operator[](const std::size_t blockNum)
{
	return m_data[blockNum];
}

}	 // namespace common
