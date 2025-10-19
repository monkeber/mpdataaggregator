#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include <chrono>
#include <thread>

constexpr auto SHARED_MEMORY_NAME{ "/mpdataaggregator" };
constexpr auto SHARED_MEMORY_LENGTH{ 2560 };

constexpr auto DATA_CHUNK_SIZE{ 256 };

struct ShBuf
{
	pid_t pid;
	char data[DATA_CHUNK_SIZE];

	void SetData(const char* str)
	{
		std::memcpy(data, str, std::strlen(str) + 1);
	}
};

int main()
{
	std::cout << "Consumer Started" << std::endl;

	const auto fd{ shm_open(SHARED_MEMORY_NAME, O_RDONLY | O_CREAT, 0666) };

	ftruncate(fd, SHARED_MEMORY_LENGTH);

	void* ptr = mmap(NULL, SHARED_MEMORY_LENGTH, PROT_READ, MAP_SHARED, fd, 0);

	close(fd);

	std::this_thread::sleep_for(std::chrono::seconds{ 7 });

	ShBuf buf;
	std::memcpy(&buf, ptr, sizeof(ShBuf));

	std::cout << buf.pid << "\n" << buf.data << std::endl;

	munmap(ptr, SHARED_MEMORY_LENGTH);
	shm_unlink(SHARED_MEMORY_NAME);

	return 0;
}