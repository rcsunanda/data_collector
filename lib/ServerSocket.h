#pragma once

#include <BaseSocket.h>

class ServerSocket: public BaseSocket
{
public:
	
	ServerSocket();
	ServerSocket(int socketFD, SocketManager* socketMan, SocketCallback* callback, char* serverIP, int serverPort);
	~ServerSocket() { }

private:
	char m_serverIP[20];
	int m_serverPort;
};
