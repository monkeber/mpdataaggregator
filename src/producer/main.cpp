#include <fcntl.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include <common/Buffer.h>

int main()
{
	std::cout << "Producer Started" << std::endl;

	common::ShBuf buf{ 5 };

	// Create message queue
	mq_attr queueAttr;
	queueAttr.mq_flags = 0;
	queueAttr.mq_maxmsg = 1;
	queueAttr.mq_msgsize = 1;
	queueAttr.mq_curmsgs = 0;

	mqd_t mq =
		mq_open(common::MESSAGE_QUEUE_NAME, O_CREAT | O_WRONLY | O_NONBLOCK, 0666, &queueAttr);

	std::uint32_t seqnum{ 0 };
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
		const std::lock_guard<common::ShBuf> guard{ buf };
		common::DataBlock db;
		db.pid = getpid();
		db.seqnum = seqnum++;
		db.SetData("Hello");
		buf[0] = db;
		mq_send(mq, "", 0, 0);
	}

	mq_close(mq);

	return 0;
}