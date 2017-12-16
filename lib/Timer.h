#pragma once

#include <string>

class SocketManager;
class SocketCallback;

class Timer
{
public:

	Timer();
	Timer(int FD, int intervalSeconds, std::string name, SocketManager* socketMan, SocketCallback* callback);

	virtual ~Timer() { }

	int getTimerFD();
	std::string getTimerName();
	int getTimerInterval();

	void subscribe(SocketCallback* callback);
	SocketCallback* getCallback();

private:
	
	int m_timerFD;
	std::string m_timerName;
	int m_timerIntervalSeconds;

	SocketManager* m_socketMan;
	SocketCallback* m_callback;
};
