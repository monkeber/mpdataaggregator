#pragma once

#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <span>

constexpr auto SHARED_MEMORY_NAME{ "/mpdataaggregator" };
constexpr auto SHARED_MEMORY_LENGTH{ 2560 };

constexpr auto DATA_CHUNK_SIZE{ 256 };

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

struct ShBuf
{
	pthread_mutex_t mutex;
	std::span<DataBlock> data;

	void InitBuffer(const std::size_t numberOfBlocks)
	{
		data = std::span<DataBlock>(reinterpret_cast<DataBlock*>(this + 1), numberOfBlocks);
	}
};
