#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

#include <mqueue.h>
#include <sys/types.h>

namespace common
{

constexpr std::uint32_t DATA_CHUNK_SIZE{ 256 };

/*
 * Represents data which is being written by producers.
 */
struct DataBlock
{
	//! Process id of the producer that wrote the block.
	pid_t pid;
	//! 'Serial' number of the block produced by a particular producer.
	std::uint64_t seqnum;
	//! Data or readings from the producer, in our case it's random words.
	char payload[DATA_CHUNK_SIZE];

	//! Provides a convenient interface for writing text in the current data block.
	void SetData(const char* str)
	{
		std::memcpy(payload, str, std::strlen(str) + 1);
	}
};

/*
 * Provides interface for our shared memory buffer. Read and Insert methods should be called only
 * when the buffer mutex is locked, otherwise there is a risk of corrupting data.
 */
class ShBuf
{
public:
	explicit ShBuf(const std::size_t numberOfBlocks);
	~ShBuf();

	//
	// BasicLockable
	//
public:
	void lock();
	void unlock();

public:
	//! Inserts a new data block into the buffer, does nothing if it's already full.
	void Insert(const DataBlock& block);
	//! Returns true if the buffer is full.
	//! Reads and returns the data block from the buffer, if there is nothing to read - returns
	//! nullopt.
	std::optional<DataBlock> Read();

private:
	//! Creates or attaches to already created shared memory, performs mapping.
	void InitMemory(const std::size_t numberOfBlocks);
	//! Initializes pointers to data and size info, initializes mutex if necessary.
	void InitState(const std::size_t numberOfBlocks);

private:
	//! Posix mutex for syncing between processes.
	pthread_mutex_t* m_mutex;
	//! Represents an index at which the next read should occur.
	std::size_t* m_readIndex;
	//! Represents an index at which the next write should occur.
	std::size_t* m_writeIndex;
	//! Convenient interface for accessing the data blocks internally and the size of the buffer.
	std::span<DataBlock> m_data;
	//! Holds the actual size of the memory that was allocated for the buffer.
	std::size_t m_memorySize;
	//! Represents if this is the object that triggered the creation of shared memory, used to
	//! determine if we should call shm_unlink on destruction.
	bool m_isCreator;
};

/*
 * Provides interface for for sending and receiving messages to/from othger processes.
 */
class MQueue
{
public:
	//! By default queue will open with just O_CREAT (will try with O_EXCL first), you must provide
	//! additional flags such as O_WRONLY or O_NONBLOCK depending on your needs.
	explicit MQueue(const int additionalFlags);
	~MQueue();

public:
	//! Retrieves the oldest message from the queue, may block, depends on whether the O_NONBLOCK
	//! was passed during construction.
	void ReceiveNotify();
	//! Sends a new message into the queue, may block, depends on whether the O_NONBLOCK was
	//! passed during construction.
	void SendNotify();

private:
	//! Represents if this is the object that triggered the creation of the queue, used to
	//! determine if we should call mq_unlink on destruction.
	bool m_isCreator;
	//! Holds the queue descriptor for referencing the queue in subsequent calls.
	mqd_t m_queueDesc;
};

}	 // namespace common
