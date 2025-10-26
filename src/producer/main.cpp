#include <fcntl.h>
#include <sys/wait.h>

#include <thread>

#include <common/Core.h>
#include <common/Utils.h>

int main(int argc, char* argv[])
try
{
	const auto arguments{ common::ParseProducerArguments(argc, argv) };
	std::vector<pid_t> children;
	for (std::size_t i = 0; i < (arguments.numberOfProcesses - 1); ++i)
	{
		const auto pid{ fork() };
		if (0 == pid)
		{
			break;
		}
		else
		{
			children.push_back(pid);
		}
	}

	common::Log("Producer: Started");
	common::InitSignalHandlers();

	common::ShBuf buf{ arguments.bufferSizeInBlocks };
	common::MQueue mq{ O_WRONLY | O_NONBLOCK };

	std::uint32_t seqnum{ 0 };
	while (!common::ShouldExit())
	{
		std::this_thread::sleep_for(common::GenerateRandomInterval());
		const std::lock_guard<common::ShBuf> guard{ buf };
		if (buf.IsFull())
		{
			common::Log("Sending notify...");
			mq.SendNotify();
			continue;
		}

		common::DataBlock db;
		db.pid = getpid();
		db.seqnum = seqnum++;

		const std::string incomingData{ common::GenerateRandomString() };
		db.SetData(incomingData.c_str());

		buf.Insert(db);
	}

	if (!children.empty())
	{
		common::Log("Waiting for child processes to exit...");
		common::TerminateAllChildren();
	}

	while (!children.empty())
	{
		int* stat_lock;
		pid_t pid = wait(stat_lock);
		if (-1 == pid && errno == EINTR)
		{
			continue;
		}
		if (-1 == pid && errno == ECHILD)
		{
			break;
		}
		else
		{
			common::Log("Error while trying to wait for children: {}", strerror(errno));
			break;
		}
		std::ignore = std::remove(children.begin(), children.end(), pid);
		common::Log("Child {} exited", pid);
	}

	common::Log("Producer: Graceful exit...");

	return 0;
}
catch (const common::HelpException& e)
{
	common::Log("{}", e.what());
	return 0;
}
catch (const std::exception& e)
{
	common::Log("Error encountered:\n{}", e.what());
	return 1;
}
catch (...)
{
	common::Log("Unknown error encountered");
	return 1;
}
