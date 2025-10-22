#include <fcntl.h>

#include <iostream>
#include <thread>

#include <common/Buffer.h>

int main()
{
	std::cout << "Producer Started" << std::endl;

	common::initSignalHandlers();

	common::ShBuf buf{ 5 };
	common::MQueue mq{ O_WRONLY | O_NONBLOCK };

	std::uint32_t seqnum{ 0 };
	while (!common::shouldExit())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
		const std::lock_guard<common::ShBuf> guard{ buf };
		common::DataBlock db;
		db.pid = getpid();
		db.seqnum = seqnum++;
		db.setData("Hello");
		buf[0] = db;
		mq.sendNotify();
	}

	std::cout << "Producer: Graceful exit..." << std::endl;

	return 0;
}