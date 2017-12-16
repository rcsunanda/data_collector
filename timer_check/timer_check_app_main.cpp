//compile with CMake

#include <Logger.h>
#include <SocketCommunication.h>

class TimerCheckApp: public SocketCallback
{
public:
	TimerCheckApp() {}
	~TimerCheckApp() {}

	void initialize()
	{
		m_heartbeatTimer = m_socketMan.createTimer(3, "Heartbeat Timer", this);
	}

	void enterRunLoop()
	{
		m_socketMan.run();
	}

	void OnTimer(Timer* timer) override
	{
		++m_count;
		BOOST_LOG_TRIVIAL(info) << "Timer fired; name=" << timer->getTimerName() << "; count=" << m_count;
	}

private:
	SocketManager m_socketMan;

	int m_count = 0;
	Timer* m_heartbeatTimer = nullptr;
};



int main()
{
    BOOST_LOG_TRIVIAL(info) << "Started";

	TimerCheckApp myApp;
	myApp.initialize();
	myApp.enterRunLoop();

    return 1;
}
