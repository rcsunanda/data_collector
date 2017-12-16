#pragma once

#include <string>
#include <SocketCallback.h>
class ClientSocket;
class Forwarder: public SocketCallback
{
public:

	Forwarder() {}
	~Forwarder() {}

	//Client side callbacks
	virtual void OnDisconnect(ClientSocket* client);
	virtual void OnData(ClientSocket* client, std::string message);
};
