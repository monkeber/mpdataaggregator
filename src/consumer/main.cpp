#include <fcntl.h>

#include <mutex>

#include <common/Core.h>
#include <common/Utils.h>

int main(int argc, char* argv[])
try
{
	const auto bufferSize{ common::ParseConsumerArguments(argc, argv) };
	common::Log("Consumer: Started");
	common::InitSignalHandlers();

	common::ShBuf buf{ bufferSize };
	common::MQueue mq{ O_RDONLY };

	while (true)
	{
		mq.ReceiveNotify();

		// Interrupts can interfere with waiting for a message in a queue, so to avoid processing
		// additional data blocks before exiting we are going to check the variable after we
		// finished waiting for a message.
		if (common::ShouldExit())
		{
			break;
		}

		std::vector<common::DataBlock> readData;

		{
			const std::lock_guard<common::ShBuf> guard{ buf };
			std::optional<common::DataBlock> newElement{ buf.Read() };
			while (newElement.has_value())
			{
				readData.push_back(newElement.value());
				newElement = buf.Read();
			}
		}

		for (const auto db : readData)
		{
			common::Log("{} {} {}", db.pid, db.seqnum, db.payload);
		}
		common::Log("Number of elements read: {}", readData.size());
	}

	common::Log("Consumer: Graceful exit...");

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
