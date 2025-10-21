#pragma once

#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <span>

namespace common
{

constexpr auto DATA_CHUNK_SIZE{ 256 };
constexpr auto MESSAGE_QUEUE_NAME{ "/mpdataaggregatorqueue" };
constexpr auto SHARED_MEMORY_NAME{ "/mpdataaggregator" };
constexpr auto SHARED_MEMORY_LENGTH{ 2560 };

struct DataBlock
{
	pid_t pid;
	std::uint64_t seqnum;
	char data[DATA_CHUNK_SIZE];

	void SetData(const char* str)
	{
		std::memcpy(data, str, std::strlen(str) + 1);
	}
};

/*

*/
class ShBuf
{
public:
	ShBuf(const std::size_t numberOfBlocks);
	~ShBuf();

public:
	void lock();
	void unlock();

public:
	DataBlock& operator[](const std::size_t blockNum);

private:
	pthread_mutex_t* m_mutex;
	std::span<DataBlock> m_data;
};

}	 // namespace common
