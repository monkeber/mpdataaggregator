#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <thread>

#include <common/Buffer.h>

int main()
{
	std::cout << "Producer Started" << std::endl;

	const auto fd{ shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666) };

	ftruncate(fd, SHARED_MEMORY_LENGTH);

	ShBuf* buf =
		reinterpret_cast<ShBuf*>(mmap(NULL, SHARED_MEMORY_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0));
	close(fd);

	if (buf == MAP_FAILED)
	{
		std::cout << "Mmap returned null, errno: " << errno << std::endl;
		return 1;
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&buf->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	std::uint32_t seqnum{ 0 };
	buf->InitBuffer(5);
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
		pthread_mutex_lock(&buf->mutex);
		DataBlock db;
		db.pid = getpid();
		db.seqnum = seqnum++;
		db.SetData("Hello");
		buf->data[0] = db;
		pthread_mutex_unlock(&buf->mutex);
	}

	munmap(buf, SHARED_MEMORY_LENGTH);
	shm_unlink(SHARED_MEMORY_NAME);

	return 0;
}