#pragma once

class SocketManager;
class SocketCallback;

class BaseSocket
{
public:
	
	BaseSocket():
		m_socketFD{-1},
		m_socketMan{nullptr},
		m_callback{nullptr}
	  {}

	BaseSocket(int socketFD, SocketManager* socketMan, SocketCallback* callback):
		m_socketFD{socketFD},
		m_socketMan{socketMan},
		m_callback{callback}
	  {}

	virtual ~BaseSocket() { }

	void subscribe(SocketCallback* callback) { m_callback = callback; }

	int getSocketFD() { return m_socketFD; }

	SocketCallback* getCallback() { return m_callback; }
	SocketManager* getSocketManager() { return m_socketMan; }

protected:
	
	int m_socketFD;

private:
	
	SocketManager* m_socketMan;
	SocketCallback* m_callback;
};
