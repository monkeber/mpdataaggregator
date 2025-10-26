#include <common/Utils.h>

#include <signal.h>

#include <atomic>
#include <random>

namespace
{
namespace details
{

static std::atomic<bool> ShouldExit{ false };

}	 // namespace details
}	 // namespace

namespace common
{

void InitSignalHandlers()
{
	struct sigaction psa;
	psa.sa_handler = SignalHandler;
	sigaction(SIGTERM, &psa, NULL);
	sigaction(SIGINT, &psa, NULL);
}

void SignalHandler(int signo)
{
	switch (signo)
	{
	case SIGTERM:
	case SIGINT:
		details::ShouldExit = true;
		break;
	}
}

bool ShouldExit()
{
	return details::ShouldExit.load();
}

void TerminateAllChildren()
{
	kill(0, SIGTERM);
}

std::chrono::milliseconds GenerateRandomInterval()
{
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib{ 1, 500 };

	return std::chrono::milliseconds{ distrib(gen) };
}

std::string GenerateRandomString()
{
	static const std::array<std::string, 10> dictionary{
		"Hello",   "Otter",	  "Rat",   "Data",	"Aggregator",
		"Process", "Threads", "Mutex", "Queue", "Lock",
	};

	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib{ 0, 9 };
	std::string tempString;
	for (std::size_t i = 0; i < 5; ++i)
	{
		tempString += dictionary[distrib(gen)];
	}

	return tempString;
}

ProducerConfig ParseProducerArguments(int argc, char* argv[])
{
	const std::string helpDesc{
		R"__(
Correct usage:
	producer [-M 10] [-N 3]

Where:
	-M - number of data blocks in the buffer
	-N - number of producer processes to start
)__"
	};
	ProducerConfig ret;
	const std::vector<std::string_view> args{ argv + 1, argv + argc };
	if (args.empty())
	{
		return ret;
	}
	else if (args.size() == 1 && args.at(0) == "-h")
	{
		throw HelpException{ helpDesc };
	}
	else if (!(args.size() == 2 || args.size() == 4))
	{
		throw HelpException{ fmt::format(
			"Invalid number of parameters: {}\n{}", args.size(), helpDesc) };
	}

	for (std::size_t i = 0; i < args.size(); i += 2)
	{
		if (args.at(i) == "-M")
		{
			std::from_chars(args.at(i + 1).begin(), args.at(i + 1).end(), ret.bufferSizeInBlocks);
		}
		else if (args.at(i) == "-N")
		{
			std::from_chars(args.at(i + 1).begin(), args.at(i + 1).end(), ret.numberOfProcesses);
		}
		else
		{
			throw HelpException{ fmt::format("Invalid parameter: {}\n{}", args.at(i), helpDesc) };
		}
	}

	return ret;
}

std::uint16_t ParseConsumerArguments(int argc, char* argv[])
{
	const std::string helpDesc{
		R"__(
Correct usage:
	consumer [-M 10]

Where:
	-M - number of data blocks in the buffer
)__"
	};
	std::uint16_t numOfBlocks{ 10 };
	const std::vector<std::string_view> args{ argv + 1, argv + argc };
	if (args.empty())
	{
		return numOfBlocks;
	}
	else if (args.size() == 1 && args.at(0) == "-h")
	{
		throw HelpException{ helpDesc };
	}
	else if (args.size() != 2)
	{
		throw HelpException{ fmt::format(
			"Invalid number of parameters: {}\n{}", args.size(), helpDesc) };
	}

	for (std::size_t i = 0; i < args.size(); i += 2)
	{
		if (args.at(i) == "-M")
		{
			std::from_chars(args.at(i + 1).begin(), args.at(i + 1).end(), numOfBlocks);
		}
		else
		{
			throw HelpException{ fmt::format("Invalid parameter: {}\n{}", args.at(i), helpDesc) };
		}
	}

	return numOfBlocks;
}

}	 // namespace common
