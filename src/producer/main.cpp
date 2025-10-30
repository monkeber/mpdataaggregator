#include <fcntl.h>
#include <sys/wait.h>

#include <thread>

#include <common/Core.h>
#include <common/Utils.h>

void CreateChildren(const common::ProducerConfig& config, std::vector<pid_t>& children)
{
	for (std::size_t i = 0; i < (config.numberOfProcesses - 1); ++i)
	{
		const auto pid{ fork() };
		if (0 == pid)
		{
			// So child processes after the first one do not think they have spawned other
			// processes.
			children.clear();
			break;
		}
		else if (-1 == pid)
		{
			common::Log("Failed to create a child: {}", strerror(errno));
		}
		else
		{
			children.push_back(pid);
		}
	}
}

void TerminateChildren(std::vector<pid_t>& children)
{
	if (!children.empty())
	{
		std::string childrenstr;
		for (const auto pid : children)
		{
			childrenstr.append(std::to_string(pid));
			childrenstr.append(" ");
		}
		common::Log("Waiting for child processes to exit... PIDs: {}", childrenstr);
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
		else if (-1 == pid && errno == ECHILD)
		{
			break;
		}
		else if (-1 == pid)
		{
			common::Log("Error while trying to wait for children: {}", strerror(errno));
			break;
		}
		std::ignore = std::remove(children.begin(), children.end(), pid);
		common::Log("Child {} exited", pid);
	}
}

int main(int argc, char* argv[])
try
{
	const auto arguments{ common::ParseProducerArguments(argc, argv) };
	std::vector<pid_t> children;
	CreateChildren(arguments, children);

	common::Log("Producer: Started");
	common::InitSignalHandlers();

	common::ShBuf buf{ arguments.bufferSizeInBlocks };
	common::MQueue mq{ O_WRONLY | O_NONBLOCK };

	std::uint32_t seqnum{ 0 };
	while (!common::ShouldExit())
	{
		std::this_thread::sleep_for(common::GenerateRandomInterval());

		common::DataBlock db;
		db.pid = getpid();
		db.seqnum = seqnum++;

		const std::string incomingData{ common::GenerateRandomString() };
		db.SetData(incomingData.c_str());

		const std::lock_guard<common::ShBuf> guard{ buf };
		buf.Insert(db);
		if (buf.IsFull())
		{
			mq.SendNotify();
		}
	}

	TerminateChildren(children);

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
