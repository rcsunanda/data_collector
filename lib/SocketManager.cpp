#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> //inet_ntop
#include <sys/timerfd.h> //timerfd_create
#include <time.h> //timerfd_create
#include <ctime>
#include <unistd.h>

#include <cstring> //memset, stoi
#include <sstream>
#include <string>
#include <algorithm> //max
#include <stdint.h> // 32 bit int in union operator
#include <math.h>
#include <iostream> // testing
#include <stdexcept> // for exception handling
#include <fstream> // writing log files

#include <SocketManager.h>
#include <ServerSocket.h>
#include <ClientSocket.h>
#include <Timer.h>
#include <SocketCallback.h>
#include <Logger.h>
#include <ConfigurationHandler.h>


//*************************************************************************************************
SocketManager::SocketManager():
	m_serverSocket{nullptr},
	m_receiveBufferSize{512}, //default value if unset
	m_bufferedMessageHardLimit{8192}, //default value if unset
	m_msgTerminationCharacter{'\n'} //default value if unset
{
}


//*************************************************************************************************
SocketManager::~SocketManager()
{
	closeServerSocket();

	//Close all independent client sockets
	auto iter = m_independantClientSockets.begin();
	while (iter != m_independantClientSockets.end())
	{
		int clientFD = iter->first;
		++iter;
		closeClientSocket(clientFD);
	}

	//Close all timers
	auto iterTimers = m_timerMap.begin();
	while (iterTimers != m_timerMap.end())
	{
		int timerFD = iterTimers->first;
		++iterTimers;
		closeClientSocket(timerFD);
	}
}


//*************************************************************************************************
void SocketManager::setReceiveBufferSize(int size)
{
	m_receiveBufferSize = size;
}


//*************************************************************************************************
void SocketManager::setBufferedMessageHardLimit(int hardLimit)
{
	m_bufferedMessageHardLimit = hardLimit;
}


//*************************************************************************************************
void SocketManager::setMsgTerminationCharacter(char character)
{
	m_msgTerminationCharacter = character;
}


//*************************************************************************************************
ServerSocket* SocketManager::createServer(char* serverPort, SocketCallback* callback)
{
	//check whether a server was already created before attempting to create a new one
	if (m_serverSocket != nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "One server has been already created. Application supports only one server";
		return nullptr;
	}

	struct addrinfo addr, *info;
	memset(&addr, 0, sizeof(addrinfo));
	addr.ai_family=AF_UNSPEC;
	addr.ai_socktype=SOCK_STREAM;
	addr.ai_flags=AI_PASSIVE;

	if (getaddrinfo(NULL, serverPort, &addr, &info) !=0 )
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to get address info";
		return nullptr;
	}

	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to create socket";
		return nullptr;
	}

	if (bind(socketFD, info->ai_addr, info->ai_addrlen) == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to bind socket at " << serverPort;
		return nullptr;
	}

	freeaddrinfo(info);
	BOOST_LOG_TRIVIAL(info) << "Server socket is successfully created and bound to port " << serverPort;

	char serverIP[18];
	getLocalIP(socketFD, serverIP);

	m_serverSocket = new ServerSocket(socketFD, this, callback, serverIP, std::stoi(serverPort));

	return m_serverSocket;
}


//*************************************************************************************************
ClientSocket* SocketManager::createClient(char* remoteServerIP, char* remoteServerPort, SocketCallback* callback)
{
	struct addrinfo addr,*info;
	memset(&addr, 0, sizeof(addrinfo));
	addr.ai_family=AF_UNSPEC;
	addr.ai_socktype=SOCK_STREAM;
	addr.ai_flags=AI_PASSIVE;

	if (getaddrinfo(remoteServerIP, remoteServerPort, &addr, &info) != 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to get address info";
		return nullptr;
	}

	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to create socket";
		return nullptr;
	}

	if (::connect(socketFD, info->ai_addr, info->ai_addrlen) == -1)
	{
		closeClientSocket(socketFD);
		BOOST_LOG_TRIVIAL(error) << "Unable to connect to server " << remoteServerIP << ": " << remoteServerPort;
		return nullptr;
	}

	//Get remote and local client port
	int remoteClientPort = getRemoteClientPort(socketFD);
	int localClientPort = getLocalClientPort(socketFD);

	BOOST_LOG_TRIVIAL(info) << "Successfully created client socket and connected to server: "
			<< remoteServerIP << ":" << remoteServerPort << ", local client port: " << localClientPort
				<< ", remote client port: " << remoteClientPort;

	freeaddrinfo(info);

	//Create ClientSocket object
	//Must erase this from m_independantClientSockets vector before calling OnDisconnect
	ClientSocket* clientSocket = new ClientSocket(1, socketFD, this, callback, remoteServerIP,
						std::stoi(remoteServerPort), remoteClientPort, localClientPort, nullptr);

	m_independantClientSockets[socketFD] = clientSocket;

	return clientSocket;
}


//*************************************************************************************************
Timer* SocketManager::createTimer(int intervalSeconds, std::string timerName, SocketCallback* callback)
{
	struct itimerspec itval;
	int timerFD;

	itval.it_interval.tv_sec = intervalSeconds;	//interval
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = intervalSeconds;	//time until first fire
	itval.it_value.tv_nsec = 0;

	timerFD = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);	//Create timer

	if (timerFD == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Error creating timer FD (timerfd_create()), returned FD: " << timerFD;
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
		return nullptr;
	}

	if (timerfd_settime(timerFD, 0, &itval, NULL) == -1)	//Start timer
	{
		BOOST_LOG_TRIVIAL(error) << "Error starting timer (timerfd_settime())" << timerFD;
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
		return nullptr;
	}

	Timer* timer = new Timer(timerFD, intervalSeconds, timerName, this, callback);
	m_timerMap[timerFD] = timer;

	BOOST_LOG_TRIVIAL(info) << "Created timer, timer name: " << timerName << ", interval: " << intervalSeconds << " seconds, timer FD: " << timerFD;

	return timer;
}


//*************************************************************************************************
void SocketManager::closeClientSocket(int FD)
{
	if (close(FD) == -1)
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to close socket FD: " << FD;
		BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
	}
	FD_CLR(FD, &m_masterFDSet);
	removeClientSocket(FD);
}


//*************************************************************************************************
void SocketManager::closeServerSocket()
{
	if (m_serverSocket == nullptr)	//eg: when server has not been created
		return;

	//Close all peer client sockets created by server
	auto iter = m_peerClientSockets.begin();
	while (iter != m_peerClientSockets.end())
	{
		int peerFD = iter->first;
		++iter;
		closeClientSocket(peerFD);
	}

	closeClientSocket(m_serverSocket->getSocketFD());	//Server FD can be closed similar to a client FD
	delete m_serverSocket;
}


//*************************************************************************************************
void SocketManager::run()
{
	BOOST_LOG_TRIVIAL(info) << "SocketManager entered run loop";

	int serverSocketFD = -1;

	int fdmax = -1;

	if (m_serverSocket) //valid only for server side application
		serverSocketFD = m_serverSocket->getSocketFD();

	FD_ZERO(&m_masterFDSet);
	FD_ZERO(&m_readFDSet); //Clear the fd sets

	if (m_serverSocket)  //valid only for server side application
	{
		if (listen(serverSocketFD, 128) == -1)
		{
			BOOST_LOG_TRIVIAL(error) << "Unable to listen on socket FD: " << serverSocketFD;
			BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
			return;
		}

		FD_SET(serverSocketFD, &m_masterFDSet);
	}

	//Add each independent client socket and find max client socket FD
	for(auto& entry : m_independantClientSockets)
	{
		int clientSocketFD = entry.first;
		FD_SET(clientSocketFD, &m_masterFDSet);
		fdmax = std::max(clientSocketFD, fdmax);
	}

	//Add timer FDs and find max timer FD
	for(auto& entry : m_timerMap)
	{
		int timerFD = entry.first;
		FD_SET(timerFD, &m_masterFDSet);
		fdmax = std::max(timerFD, fdmax);
	}

	fdmax = std::max(serverSocketFD, fdmax);	//Max of above and server socket FD

	//Buffer for holding received data
	char buffer[m_receiveBufferSize];
	memset(buffer, m_msgTerminationCharacter, m_receiveBufferSize);

	char garbageBuffer[m_receiveBufferSize*10];

	while(true)
	{
		m_readFDSet = m_masterFDSet;

		if (select(fdmax+1, &m_readFDSet, NULL, NULL, NULL) == -1)
		{
			BOOST_LOG_TRIVIAL(error) << "select() failed. Application must be terminated\n";
			BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
			break;
		}

		//Run through all fds upto fdmax and see if data is available
		for (int fdi = 0; fdi <= fdmax; ++fdi)
		{
			//Check if data is available on any fdi
			if (FD_ISSET(fdi, &m_readFDSet))
			{
				if (fdi == serverSocketFD) //fdi is the server's listening socket
				{
					struct sockaddr_storage peeraddr;
					socklen_t addr_size = sizeof (peeraddr);
					int peerSocketFD = accept(serverSocketFD, (struct sockaddr*)&peeraddr, &addr_size);

					if (peerSocketFD == -1)
					{
						BOOST_LOG_TRIVIAL(error) << "Unable to accept on socket " << serverSocketFD;
						BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
					}
					else //accepted new client connection
					{
						FD_SET(peerSocketFD, &m_masterFDSet);
						if (peerSocketFD > fdmax)
							fdmax = peerSocketFD;

						//Get remote IP and remote and local client port
						char remoteIP[18];
						getRemoteIP(peerSocketFD, remoteIP);
						int remoteClientPort = getRemoteClientPort(peerSocketFD);
						int localClientPort = getLocalClientPort(peerSocketFD);

						//create ClientSocket, add it to m_peerClientSockets and fire server side OnConnect

						ClientSocket* peerClientSocket = new ClientSocket(2, peerSocketFD, this,
												m_serverSocket->getCallback(), remoteIP, -1,
												remoteClientPort, localClientPort, m_serverSocket);

						m_peerClientSockets[peerSocketFD] = peerClientSocket;
						m_serverSocket->getCallback()->OnConnect(m_serverSocket, peerClientSocket);

					}
				}
				else if (m_timerMap.count(fdi) > 0)	//fdi is a timer FD
				{
					unsigned long long queuedTimerFireCount;

					int result = read(fdi, &queuedTimerFireCount, sizeof(queuedTimerFireCount));

					if (result == -1)
					{
						//since the timer is non-blocking, it may be unable to perform a successful read without blocking (sets EWOULDBLOCK or EAGAIN)
						//this is extremly rare because at this point, there is data to be read in the timer FD (as we have come here from the select() call)
						//but we handle this possibility as a possible fix to the block-on-read bug (blocking on __read_nocancel)
						if (errno == EWOULDBLOCK || errno == EAGAIN)	
						{
							BOOST_LOG_TRIVIAL(error) << "read() error on timer file descriptor: " << fdi << "; cannot read() without blocking";
							BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
						}
						else	//errors other than would-block
						{
							BOOST_LOG_TRIVIAL(error) << "read() error on timer file descriptor. Exiting program " << fdi;
							BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
							close(fdi);
							FD_CLR(fdi, &m_masterFDSet);
							exit(0);	//since we have several critical timers, losing one of them means that we cannot continue running the program
						}
					}
					else
					{
						Timer* timer = m_timerMap[fdi];
						timer->getCallback()->OnTimer(timer);
					}
				}
				else //fdi is a client socket
				{

					int length = recv(fdi, buffer, m_receiveBufferSize, 0);

					if (length == -1) //Error receiving
					{
						BOOST_LOG_TRIVIAL(error) << "recv() error on peer client socket " << fdi;
						BOOST_LOG_TRIVIAL(error) << "errno: " << errno << ", error string: " << strerror(errno);
						closeClientSocket(fdi);
					}
					else if (length > 0) //Data was received
					{
						//Form a valid message here and fire OnData callback
						parseReceivedData(fdi, buffer, length);
						//memset(buffer, m_msgTerminationCharacter, m_receiveBufferSize); //Unnecessary
					}
					else if (length == 0) //Client disconnected
					{
						ClientSocket* clientSocket = getClientSocket(fdi);

						if (clientSocket->getClientType() == 1) //client side client
							clientSocket->getCallback()->OnDisconnect(clientSocket);

						if (clientSocket->getClientType() == 2) //server side client
							clientSocket->getCallback()->OnDisconnect(m_serverSocket, clientSocket);

						//Receive any more remaining (buffered) data.
						//This may be unnecessary, but just in case for when data is sent to server at very high rates
						struct timeval tv;
						tv.tv_sec = 1;
						tv.tv_usec = 0;

						//Timeout for the typical case when no data is buffered
						setsockopt(fdi, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

						closeClientSocket(fdi);
					}
				}
			} //End if (data available on any fdi)
		} //End for (all fds upto fdmax)
	} //End while(true)

	close(serverSocketFD);

	BOOST_LOG_TRIVIAL(info) << "Exiting server (Run loop has ended)";
}


//*************************************************************************************************
int SocketManager::getLocalClientPort(int socketFD)
{
	struct sockaddr_in local_address;
	int local_addr_size = sizeof(local_address);
	getsockname(socketFD, (struct sockaddr *)&local_address, (socklen_t*)&local_addr_size);

	int localClientPort = (int) ntohs(local_address.sin_port);
	return localClientPort;
}


//*************************************************************************************************
int SocketManager::getRemoteClientPort(int socketFD)
{
	struct sockaddr_in remote_address;
	int remote_addr_size = sizeof(remote_address);
	getpeername(socketFD, (struct sockaddr *)&remote_address, (socklen_t*)&remote_addr_size);

	int remoteClientPort = (int) ntohs(remote_address.sin_port);
	return remoteClientPort;
}


//*************************************************************************************************
void SocketManager::getLocalIP(int socketFD, char* returnIP)
{
	struct sockaddr_in local_address;
	int local_addr_size = sizeof(local_address);
	getsockname(socketFD, (struct sockaddr *)&local_address, (socklen_t*)&local_addr_size);

	char localIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(local_address.sin_addr), localIP, INET_ADDRSTRLEN);

	strcpy(returnIP, localIP);
}


//*************************************************************************************************
void SocketManager::getRemoteIP(int socketFD, char* returnIP)
{
	struct sockaddr_in remote_address;
	int remote_addr_size = sizeof(remote_address);
	getpeername(socketFD, (struct sockaddr *)&remote_address, (socklen_t*)&remote_addr_size);

	char remoteIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(remote_address.sin_addr), remoteIP, INET_ADDRSTRLEN);

	strcpy(returnIP, remoteIP);
}


//*************************************************************************************************
void SocketManager::removeClientSocket(int socketFD)
{
	//Remove from m_peerClientSockets if fdi was a peer client socket
	if (m_peerClientSockets.count(socketFD) > 0)
	{
		delete m_peerClientSockets.at(socketFD);
		m_peerClientSockets.erase(socketFD);
	}
	//Remove from m_independantClientSockets if fdi was an independant client socket
	else if (m_independantClientSockets.count(socketFD) > 0)
	{
		delete m_independantClientSockets.at(socketFD);
		m_independantClientSockets.erase(socketFD);
	}
	//Remove from m_timerMap if fdi was a timer
	else if (m_timerMap.count(socketFD) > 0)
	{
		delete m_timerMap.at(socketFD);
		m_timerMap.erase(socketFD);
	}

	//Remove from message map
	if (m_messageMap.count(socketFD) > 0)
	{
		m_messageMap.erase(socketFD);
	}
}


//*************************************************************************************************
ClientSocket* SocketManager::getClientSocket(int socketFD)
{
	//Find in m_peerClientSockets
	if (m_peerClientSockets.count(socketFD) > 0)
	{
		return m_peerClientSockets.at(socketFD);
	}

	//Find in m_independantClientSockets
	if (m_independantClientSockets.count(socketFD) > 0)
	{
		return m_independantClientSockets.at(socketFD);
	}
}

//!TODO move this to a base class
//*************************************************************************************************
// Helper function to get current date and time
std::string SocketManager::getCurrentDateTime(){
	std::time_t rawtime;
    std::tm* timeinfo;
    char buffer [80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    std::strftime(buffer,80,"%Y-%m-%d %H-%M-%S",timeinfo);
    std::string timeString(buffer);
    return timeString;
}


//!TODO move this to a base class
//************************************************************************************************
// convert time integer into time string
std::string SocketManager::timeStampToHReadble(time_t rawtime)
{
    struct tm * dt;
    char buffer [30];
    dt = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", dt);
    return std::string(buffer);
}
//!TODO move this to a base class
//************************************************************************************************

std::vector<std::string> SocketManager::splitString(std::string input, char delimeter)
{
	std::vector<std::string> result;

	std::stringstream stringStream(input);
	std::string element;

	while (std::getline(stringStream, element, delimeter))
	{
		element.erase(0, element.find_first_not_of(' '));	//Left trim
		element.erase(element.find_last_not_of(' ') + 1);	//Right trim

		if (element.empty())
			continue;

		result.push_back(element);
	}
	return result;
}


//************************************************************************************************

// !TODO the following content should be moved elsewhere
#define SSTR( x ) dynamic_cast<std::ostringstream & >( \
        ( std::ostringstream() << x ) ).str()

std::string SocketManager::decodeMsg(std::vector<char>* buffer){ // retuns a general formatted string

	union savedFloat{
   		char buf[4];
   		float number;
	}savedFloat;
	union savedLong{
   		char buf[4];
   		int32_t number;
	}savedLong;

	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();
	std::string dataRecordTypeString = configHandler.getConfig("DataRecordType");
	std::vector<std::string> dataRecordType = splitString(dataRecordTypeString, ','); // should be loaded from the config file
	std::string stringRecord="";
	std::string stringVar=""; // variable extracted
	int binaryRecordSize = std::stoi(configHandler.getConfig("BinaryDataSize"));//should be loaded from the config file
	int counter=0;
	int jump=0,jumpSum=0,i=0,j=0,k=0;
	unsigned char check_sum;
    std::vector<char>::iterator lastBreakingPosition=buffer->begin();
	while(i<=buffer->size()){
		if(jumpSum == binaryRecordSize)
		{
		    // binary data log

		    std::ofstream binaryLog;
            binaryLog.open ("binary.log",std::ios::app);

            savedLong.buf[0] = (unsigned char)buffer->at(i-120);
            savedLong.buf[1] = (unsigned char)buffer->at(i-119);
            savedLong.buf[2] = (unsigned char)buffer->at(i-118);
            savedLong.buf[3] = (unsigned char)buffer->at(i-117);

            binaryLog << "Device Id: " << savedLong.number << " " << getCurrentDateTime() << "\n";

            if(i-128 >= 0){
                for(k=i-128;k<i;k++)
                {
                   savedLong.buf[0] = (unsigned char)buffer->at(k);
                   savedLong.buf[1] = 0;
                   savedLong.buf[2] = 0;
                   savedLong.buf[3] = 0;
                   binaryLog << savedLong.number;
                   if((k-i+129)%4)
                    binaryLog << " ";
                   else
                    binaryLog << " , ";
                }
            }

            binaryLog << "\n";

            binaryLog.close();

		    // end binary data log

			jumpSum=0;
			j=0;
			if((unsigned char)buffer->at(i-1) ==  255)
			{
			    check_sum = (unsigned char)0;
			    for(k=i-3;k>=i-128 && k>=0;k--)
                    check_sum = check_sum^(unsigned char)buffer->at(k);

                if(check_sum == (unsigned char)buffer->at(i-2))
                {
                    stringRecord =  stringRecord + stringVar+"\n";
                    lastBreakingPosition=buffer->begin()+i;
                }
			}
			if(i==buffer->size())
            {
                buffer->erase(buffer->begin(),buffer->end());
                return stringRecord;
            }

			stringVar="";
		}
		if(dataRecordType[j] == "esc")
		{
				jump= 4;
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}

		}
		else if(dataRecordType[j] == "int")
		{
				jump=4;
				for(k=0;k<sizeof(savedLong.buf) && i+k<buffer->size();k++)
                    savedLong.buf[k]=buffer->at(i+k);
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}
				if(!isnan(savedLong.number))
					stringVar = stringVar + SSTR(savedLong.number)+",";
				else
					stringVar = stringVar + "NAN,";
		}
		else if(dataRecordType[j] == "float")
		{
				jump=4;
			    for(k=0;k<sizeof(savedFloat.buf)&& i+k<buffer->size();k++)
                    savedFloat.buf[k]=buffer->at(i+k);
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}
				if(!isnan(savedFloat.number))
					stringVar = stringVar + SSTR(savedFloat.number)+",";
				else
					stringVar = stringVar + "NAN,";
		}
		else if(dataRecordType[j] == "date_time")
		{
				jump=4;
			    for(k=0;k<sizeof(savedLong.buf)&& i+k<buffer->size();k++)
                    savedLong.buf[k]=buffer->at(i+k);
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}
				if(!isnan(savedLong.number))
					stringVar = stringVar + timeStampToHReadble(savedLong.number-5*3600-30*60)+",";
				else
					stringVar = stringVar + "NAN,";

		}
		else if(dataRecordType[j] == "char")
		{
				jump=1;
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}
				savedLong.buf[0]=buffer->at(i);
				savedLong.buf[1]=0;
				savedLong.buf[2]=0;
				savedLong.buf[3]=0;
				stringVar = stringVar + SSTR(savedLong.number)+",";
		}
		else if(dataRecordType[j] == "end")
		{
				jump=1;
				if(i+jump>=buffer->size())
				{
				    buffer->erase(buffer->begin(),lastBreakingPosition);
					return stringRecord;
				}
		}
		j++;
		i+=jump;
		jumpSum+=jump;
	}
	buffer->erase(buffer->begin(),lastBreakingPosition);
	return stringRecord;
}



//*************************************************************************************************
void SocketManager::parseReceivedData(int socketFD, char* dataBuffer, int dataLength)
{
	//Preconditions to be met
	if (dataLength < 1)
		return;

	std::vector<char>* messageVec = &m_messageMap[socketFD];
	messageVec->insert(messageVec->end(), &dataBuffer[0], &dataBuffer[dataLength]);

	// New code to convert from binary. !TODO this must be changed.
	std::string binaryEncodedData(m_messageMap[socketFD].begin(),m_messageMap[socketFD].end());
	BOOST_LOG_TRIVIAL(debug) << "Decoding";
    std::string decodedData = decodeMsg(&m_messageMap[socketFD]);
//	std::string decodedData = decodeMsg(binaryEncodedData);
    //***********************
//	m_messageMap[socketFD].clear();
//	std::copy(decodedData.begin(), decodedData.end(), std::back_inserter(m_messageMap[socketFD]));
	//***********************
	// End convert from binary

	/*
	//Print copied vector of chars to verify that data was received properly
	for (auto& character: *messageVec)
	{
		BOOST_LOG_TRIVIAL(debug) << character;
	}
	BOOST_LOG_TRIVIAL(debug) << " ";
	*/

	//Find possible records
	while(true)
	{
		//Find the first occurrence of the record terminating character (in the now remaining vector)
		auto iter = find (decodedData.begin(), decodedData.end(), m_msgTerminationCharacter);

		if (iter != decodedData.end()) //Found an occurrence
		{
			//Construct a message from data up to the terminating character and erase that part from the vector
			std::string fullMessage(decodedData.begin(), iter);
			decodedData.erase(decodedData.begin(), ++iter);
			//Fire OnData callback with the message
			ClientSocket* clientSocket = getClientSocket(socketFD);

			if (clientSocket->getClientType() == 1) //client side client
				clientSocket->getCallback()->OnData(clientSocket, fullMessage);

			if (clientSocket->getClientType() == 2) //server side client
				clientSocket->getCallback()->OnData(m_serverSocket, clientSocket, fullMessage);

			//m_messageMap may have been affected in callbacks (eg: entry removed due to unknown device)
			//Therefore check whether the FD exists in the map
			if(m_messageMap.count(socketFD) == 0)
				break;
		}
		else //Did not find an occurrence
		{
			break;
		}
	}
}

