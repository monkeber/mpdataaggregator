#include <fcntl.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <mutex>

#include <common/Buffer.h>

int main()
{
	std::cout << "Consumer Started" << std::endl;

	common::ShBuf buf{ 5 };

	// Create message queue
	mq_attr queueAttr;
	queueAttr.mq_flags = 0;
	queueAttr.mq_maxmsg = 1;
	queueAttr.mq_msgsize = 1;
	queueAttr.mq_curmsgs = 0;

	mqd_t mq = mq_open(common::MESSAGE_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &queueAttr);

	while (true)
	{
		char msg[1];
		mq_receive(mq, msg, 1, nullptr);

		const std::lock_guard<common::ShBuf> guard{ buf };
		common::DataBlock db = buf[0];
		std::cout << db.pid << " " << db.seqnum << " " << db.data << std::endl;
	}

	mq_close(mq);
	mq_unlink(common::MESSAGE_QUEUE_NAME);

	return 0;
}