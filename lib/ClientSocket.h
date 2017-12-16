#pragma once

#include <BaseSocket.h>

#include <string>

class ServerSocket;
class SocketCallback;
class SocketManager;

class ClientSocket: public BaseSocket
{
public:
	ClientSocket();
	ClientSocket(int clientType, int socketFD, SocketManager* socketMan, SocketCallback* callback,
						char* remoteServerIP, int remoteServerPort, int remoteClientPort, 
						int localClientPort, ServerSocket* ownerServer);
	~ClientSocket() {}

	int getClientType() { return m_clientType; }
	std::string getRemoteIP() { return m_remoteIP; }
	int getRemoteServerPort() { return m_remoteServerPort; }
	int getRemoteClientPort() { return m_remoteClientPort; }
	int getLocalClientPort() { return m_localClientPort; }

	bool sendData(std::string data);

private:
	
	int m_clientType; //1 = independently created client, 2 = peer client created by server
	
	//IP of remote end (could be a server for an independent client, or a client for a peer client socket)
	char m_remoteIP[20]; 
	
	int m_remoteServerPort; //-1 for peer clients accepted by a server socket
	int m_remoteClientPort;
	int m_localClientPort;

	ServerSocket* m_ownerServer;
};
