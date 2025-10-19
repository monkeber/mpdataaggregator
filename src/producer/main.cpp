#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>

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
	std::cout << "Producer Started" << std::endl;

	const auto fd{ shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT, 0666) };

	ftruncate(fd, SHARED_MEMORY_LENGTH);

	void* ptr = mmap(NULL, SHARED_MEMORY_LENGTH, PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	if (ptr == MAP_FAILED)
	{
		std::cout << "Mmap returned null, errno: " << errno << std::endl;
		return 1;
	}

	ShBuf buf;
	buf.pid = getpid();
	buf.SetData("Hello");

	std::memcpy(ptr, &buf, sizeof(ShBuf));

	munmap(ptr, SHARED_MEMORY_LENGTH);
	shm_unlink(SHARED_MEMORY_NAME);

	return 0;
}