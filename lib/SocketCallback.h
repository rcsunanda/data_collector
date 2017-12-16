#pragma once

#include <string>

class ClientSocket;
class ServerSocket;
class Timer;

class SocketCallback
{
public:
	
	//Client side callbacks
	virtual void OnDisconnect(ClientSocket* client) {}
	virtual void OnData(ClientSocket* client, std::string message) {}

	//Server side callbacks
	virtual void OnConnect(ServerSocket* server, ClientSocket* client) {} //client = which client socket connected
	virtual void OnDisconnect(ServerSocket* server, ClientSocket* client) {}
	virtual void OnData(ServerSocket* server, ClientSocket* client, std::string message) {}

	//Timer callback
	virtual void OnTimer(Timer* timer) {}
};
