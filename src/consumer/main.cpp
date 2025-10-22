#include <fcntl.h>

#include <iostream>
#include <mutex>

#include <common/Buffer.h>

int main()
{
	std::cout << "Consumer Started" << std::endl;

	common::ShBuf buf{ 5 };
	common::MQueue mq{ O_RDONLY, true };

	while (true)
	{
		mq.ReceiveNotify();

		const std::lock_guard<common::ShBuf> guard{ buf };
		for (const auto& db : buf)
		{
			std::cout << db.pid << " " << db.seqnum << " " << db.data << std::endl;
		}
		buf.ResetData();
	}

	return 0;
}