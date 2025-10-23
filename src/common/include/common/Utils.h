#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace common
{

//
// Handling of system signals that the processes can receive.
//

//! Initializes signal handlers. Should be called at the start of process execution.
void InitSignalHandlers();
//! Used by both producer and consumer to properly handly interrupts.
void SignalHandler(const int signo);
//! Used to check if the process main loop should break.
bool ShouldExit();

//
// Generating random data.
//

//! Generates a random string from a list of predetermined words.
std::string GenerateRandomString();

//! Generates a random interval to simulate producer reading some data.
std::chrono::milliseconds GenerateRandomInterval();

//
// Logging.
//

//! Primitive logging functionality.
template<typename... Args>
auto Log(fmt::format_string<Args...> fmt, Args&&... args)
{
	return fmt::println(fmt, std::forward<Args>(args)...);
}

//
// Command line parameters.
//

//
// Allows to notify clients about correct program usage.
//
class HelpException : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

struct ProducerConfig
{
	//! Number of producer processes, deafault is 1.
	std::uint16_t numberOfProcesses{ 1 };
	//! Size of the shared buffer in blocks, should be equal to the one passed to consumer. Default
	//! is 10.
	std::uint16_t bufferSizeInBlocks{ 10 };
};
//! Parses command line parameters intended for producer process.
ProducerConfig ParseProducerArguments(int argc, char* argv[]);

//! Parses comman line parameteres intended for consumer process, for now it is only the buffer
//! size.
std::uint16_t ParseConsumerArguments(int argc, char* argv[]);

}	 // namespace common
