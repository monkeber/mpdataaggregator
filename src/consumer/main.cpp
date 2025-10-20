#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include <common/Buffer.h>

int main()
{
	std::cout << "Consumer Started" << std::endl;

	const auto fd{ shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666) };

	ftruncate(fd, SHARED_MEMORY_LENGTH);

	ShBuf* buf = reinterpret_cast<ShBuf*>(mmap(NULL, SHARED_MEMORY_LENGTH, PROT_READ, MAP_SHARED, fd, 0));
	close(fd);

	// buf->InitBuffer(5);
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
		pthread_mutex_lock(&buf->mutex);
		DataBlock db = buf->data[0];
		std::cout << db.pid << " " << db.seqnum << " " << db.data << std::endl;
		pthread_mutex_unlock(&buf->mutex);
	}

	munmap(buf, SHARED_MEMORY_LENGTH);
	shm_unlink(SHARED_MEMORY_NAME);

	return 0;
}