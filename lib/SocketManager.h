#pragma once

#include <unordered_map>
#include <vector>

class ServerSocket;
class ClientSocket;
class Timer;
class SocketCallback;

class SocketManager
{
public:
	SocketManager();
	~SocketManager();

	void setReceiveBufferSize(int size);
	void setBufferedMessageHardLimit(int hardLimit);
	void setMsgTerminationCharacter(char character);

	ServerSocket* createServer(char* serverPort, SocketCallback* callback);
	ClientSocket* createClient(char* remoteServerIP, char* remoteServerPort, SocketCallback* callback);
	Timer* createTimer(int intervalSeconds, std::string timerName, SocketCallback* callback);

	void closeClientSocket(int FD);
	void closeServerSocket();

	void run(); //main run loop of a thread

private:
	//Helper functions
	static int getLocalClientPort(int socketFD);
	static int getRemoteClientPort(int socketFD);
	static void getLocalIP(int socketFD, char* returnIP);
	static void getRemoteIP(int socketFD, char* returnIP);

	void removeClientSocket(int socketFD);
	ClientSocket* getClientSocket(int socketFD);


	// Helper function to get current date and time
	std::string getCurrentDateTime();

	// Convert timestamp integer to date string
	std::string timeStampToHReadble(time_t rawtime);

	// split string.!TODO this must be removed.
	std::vector<std::string> splitString(std::string input, char delimeter);

	// binary data decoding.!TODO this must be removed.
	std::string decodeMsg(std::vector<char>* buffer);

	//Form a valid message here and fire OnData callback
	void parseReceivedData(int socketFD, char* dataBuffer, int dataLength);


	//currently one application can create only one server --> to extened, vector of servers
	ServerSocket* m_serverSocket;

	//map of peer client sockets created by above server --> to extend, list/map of such maps
	std::unordered_map<int, ClientSocket*> m_peerClientSockets;

	//map of client sockets created independently by the application developer
	//mostly used only in client side applications
	std::unordered_map<int, ClientSocket*> m_independantClientSockets;

	//Timers created by the application
	std::unordered_map<int, Timer*> m_timerMap;

	//for data parsing --> if vector performance is unacceptable, implement a circular buffer
	std::unordered_map<int, std::vector<char>> m_messageMap;

	//Useful for receiving data and parsing them into messages
	int m_receiveBufferSize;
	int m_bufferedMessageHardLimit;
	char m_msgTerminationCharacter;

	//For socket communication
	fd_set m_masterFDSet;
	fd_set m_readFDSet;
};
