//g++ timer_check_simple_main.cpp -o timer_check_simple -lpthread -lboost_log -DBOOST_LOG_DYN_LINK

#include <cstring>	//strerror
#include <errno.h>	//errno
#include <string>
#include <cstdio>

//timerfd_create
#include <sys/timerfd.h> 
#include <time.h>

//fcntl
#include <unistd.h>
#include <fcntl.h>

//Boost logger headers
#include <boost/log/trivial.hpp>


int createTimer(int intervalSeconds, std::string timerName)
{
	struct itimerspec itval;
	int timerFD;

	itval.it_interval.tv_sec = intervalSeconds;	//interval
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = intervalSeconds;	//time until first fire
	itval.it_value.tv_nsec = 0;

	timerFD = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);	//Create timer

	if (timerFD == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Error creating timer FD (timerfd_create()), returned FD: " << timerFD;
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
		return -1;
	}

	if (timerfd_settime(timerFD, 0, &itval, NULL) == -1)	//Start timer
	{
		BOOST_LOG_TRIVIAL(error) << "Error starting timer (timerfd_settime())" << timerFD;
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
		return -1;
	}

	int FDStatusFlags = fcntl(timerFD, F_GETFL);

	BOOST_LOG_TRIVIAL(info) << "Created timer, timer name: " << timerName << ", interval: " << intervalSeconds << " seconds, timer FD: " << timerFD
		<< ", FDStatusFlags: " << FDStatusFlags;

	return timerFD;
}


int main()
{
    BOOST_LOG_TRIVIAL(info) << "Started";

	int FD1 = createTimer(5, "T1-3sec");

	if (FD1 == -1) return 1;
    
	unsigned long long queuedTimerFireCount;
	int result = 0;
	int count = 0;

	//since the timer FD is non-blocking, the while loop will execute in a spin (no blocking: high CPU usage)
    while(result = read(FD1, &queuedTimerFireCount, sizeof(queuedTimerFireCount)))
    {
		if (result == -1)
		{
			if (errno != EWOULDBLOCK && errno != EAGAIN)	//errors other than the expected would-block
				break;
		}
		else
		{
			++count;
			BOOST_LOG_TRIVIAL(info) << "Timer fired; count=" << count;
		}
    }

	//we have exited the while loop due to error in read()
	BOOST_LOG_TRIVIAL(error) << "read() error on timer file descriptor " << FD1;
	BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
	close(FD1);

    return 1;
}
