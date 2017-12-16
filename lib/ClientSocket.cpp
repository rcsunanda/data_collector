#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring> //memset, stoi

#include <ClientSocket.h>
#include <Logger.h>


//*************************************************************************************************
ClientSocket::ClientSocket():
	m_clientType{-1},
	m_remoteServerPort{-1},
	m_remoteClientPort{-1},
	m_localClientPort{-1},
	m_ownerServer{nullptr}
{
	//memset IP
	memset(m_remoteIP, '\0', sizeof(char)*20);
}


//*************************************************************************************************
ClientSocket::ClientSocket(int clientType, int socketFD, SocketManager* socketMan, SocketCallback* callback,
								char* remoteServerIP, int remoteServerPort, int remoteClientPort, 
								int localClientPort, ServerSocket* ownerServer):
	BaseSocket(socketFD, socketMan, callback),
	m_clientType{clientType},
	m_remoteServerPort{remoteServerPort},
	m_remoteClientPort{remoteClientPort},
	m_localClientPort{localClientPort},
	m_ownerServer{ownerServer}
{
	strcpy(m_remoteIP, remoteServerIP);
}


//*************************************************************************************************
bool ClientSocket::sendData(std::string data)
{
    if (::send(m_socketFD, data.c_str(), data.size(), 0) != (int)data.size())
    {
		BOOST_LOG_TRIVIAL(error) << "Unable to send data on FD: " << getSocketFD();
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
		return false;
    }

	return true;
}

