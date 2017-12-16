#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring> //memset, stoi

#include <ServerSocket.h>
#include <Logger.h>


//*************************************************************************************************
ServerSocket::ServerSocket():
	m_serverPort{-1}
{}


//*************************************************************************************************
ServerSocket::ServerSocket(int socketFD, SocketManager* socketMan, 
								SocketCallback* callback, char* serverIP, int serverPort):
	BaseSocket(socketFD, socketMan, callback),
	m_serverPort{serverPort}
{
	strcpy(m_serverIP, serverIP);
}
