#include <Timer.h>


//*************************************************************************************************
Timer::Timer():
	m_timerFD{-1},
	m_timerIntervalSeconds{-1},
	m_timerName{""},
	m_socketMan{nullptr},
	m_callback{nullptr}
{
}


//*************************************************************************************************
Timer::Timer(int FD, int intervalSeconds, std::string name,  SocketManager* socketMan, SocketCallback* callback):
		m_timerFD{FD},
		m_timerIntervalSeconds{intervalSeconds},
		m_timerName{name},
		m_socketMan{socketMan},
		m_callback{callback}
{
}


//*************************************************************************************************
void Timer::subscribe( SocketCallback* callback )
{
	m_callback = callback;
}


//*************************************************************************************************
int Timer::getTimerFD()
{
	return m_timerFD;
}


//*************************************************************************************************
std::string Timer::getTimerName()
{
	return m_timerName;
}


//*************************************************************************************************
int Timer::getTimerInterval()
{
	return m_timerIntervalSeconds;
}

//*************************************************************************************************
SocketCallback* Timer::getCallback()
{
	return m_callback;
}
