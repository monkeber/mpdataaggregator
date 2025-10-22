#pragma once

#include <cstdint>
#include <cstring>
#include <span>

#include <mqueue.h>
#include <sys/types.h>

namespace common
{

constexpr auto DATA_CHUNK_SIZE{ 256 };

/*
 * Represents a data which is being written by producers.
 */
struct DataBlock
{
	//! Process id of the producer that wrote the block.
	pid_t pid;
	//! 'Serial' number of the block produced by a particular producer.
	std::uint64_t seqnum;
	//! Data or readings from the producer, in our case it's random words.
	char data[DATA_CHUNK_SIZE];

	void setData(const char* str)
	{
		std::memcpy(data, str, std::strlen(str) + 1);
	}
};

/*
 * Provides interface for our shared memory.
 */
class ShBuf
{
public:
	ShBuf(const std::size_t numberOfBlocks);
	~ShBuf();

	//
	// BasicLockable
	//
public:
	void lock();
	void unlock();

public:
	DataBlock& operator[](const std::size_t blockNum);

private:
	pthread_mutex_t* m_mutex;
	std::span<DataBlock> m_data;
};

class MQueue
{
public:
	//! By default queue will open with just O_CREAT, you must provide additional flags such as
	//! O_WRONLY or O_NONBLOCK depending on your needs. shouldUnlink specifies if the mq_unlink
	//! should be called on object destruction.
	MQueue(const int additionalFlags, const bool shouldUnlink = false);
	~MQueue();

public:
	//! Retrieves an oldest message from the queue, may block, depends on whether the O_NONBLOCK was
	//! passed during construction.
	void receiveNotify();
	//! Sends a new message into the queue, may block, depends on whether the O_NONBLOCK was
	//! passed during construction.
	void sendNotify();

private:
	bool m_shouldUnlink;
	mqd_t m_queueDesc;
};

//
// Handling of signals that the processes can receive.
//

//! Initializes signal handlers. Should be called at the start of process execution.
void initSignalHandlers();
//! Used by both producer and consumer to properly handly interrupts.
void signalHandler(const int signo);
//! Used to check if the process main loop should break.
bool shouldExit();

}	 // namespace common
