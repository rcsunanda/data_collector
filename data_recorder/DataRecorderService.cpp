#include <cstring> //strncpy
#include <exception>
#include <fstream>	//for dumping service info to file

#include <DataRecorderService.h>
#include <DataStorage.h>
#include <ConfigurationHandler.h>
#include <Logger.h>
#include <Forwarder.h>


//*************************************************************************************************
DataRecorderService::DataRecorderService():
	m_dataRecorderServer{nullptr},
	m_heartbeatTimer{nullptr},
	m_cacheFlushTimer{nullptr},
	m_deviceLoadTimer{nullptr},
	m_dataStorage{nullptr},
	m_rejectionCount{0}
{
}


//*************************************************************************************************
bool DataRecorderService::initialize()
{
	//Create server and two timers
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	strncpy(m_servicePort, configHandler.getConfig("ServicePort").c_str(), sizeof(m_servicePort));

	int receiveBufferSize;
	int bufferedMessageHardLimit;
	int heartbeatTimerInterval;
	int cacheFlushTimerInterval;
	int deviceLoadTimerInterval;
	int nullRecordGenerationTimerInterval;
	int FDCheckTimerInterval;
	try
	{
		receiveBufferSize = std::stoi(configHandler.getConfig("ReceiveBufferSize"));
		bufferedMessageHardLimit = std::stoi(configHandler.getConfig("BufferedMessageHardLimit"));
		heartbeatTimerInterval = std::stoi(configHandler.getConfig("HeartbeatTimerInterval"));
		cacheFlushTimerInterval = std::stoi(configHandler.getConfig("CacheFlushTimerInterval"));
		deviceLoadTimerInterval = std::stoi(configHandler.getConfig("DeviceLoadTimerInterval"));
		nullRecordGenerationTimerInterval = std::stoi(configHandler.getConfig("NullRecordGenerationTimerInterval"));
		FDCheckTimerInterval = std::stoi(configHandler.getConfig("FDCheckTimerInterval"));
		m_deviceInactiveTimeThreshold = std::stoi(configHandler.getConfig("DeviceInactiveTimeThreshold"));

		m_forwarder = new Forwarder();
        m_forwardingClient = m_socketMan.createClient((char *) m_configHandler.getConfig("ForwardIP").c_str(),(char *) m_configHandler.getConfig("ForwardPort").c_str(),m_forwarder);

	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Exception thrown by std::stoi() in DataRecorderService::initialize() when reading integer configs";
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}


	char terminationCharacter = configHandler.getTerminationCharacter();

	m_socketMan.setReceiveBufferSize(receiveBufferSize);
	m_socketMan.setBufferedMessageHardLimit(bufferedMessageHardLimit);
	m_socketMan.setMsgTerminationCharacter(terminationCharacter);

	m_msgTerminationCharacter = terminationCharacter;

	m_dataRecorderServer = m_socketMan.createServer(m_servicePort, this);

	if (m_dataRecorderServer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create server for data recorder service at port: " << m_servicePort;
		return false;
	}

	m_heartbeatTimer = m_socketMan.createTimer(heartbeatTimerInterval, "Heartbeat Timer", this);

	if (m_heartbeatTimer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create heartbeat timer" << m_servicePort;
		return false;
	}

	m_cacheFlushTimer = m_socketMan.createTimer(cacheFlushTimerInterval, "Cache Flush Timer", this);

	if (m_cacheFlushTimer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create cache flush timer" << m_servicePort;
		return false;
	}

	m_deviceLoadTimer = m_socketMan.createTimer(deviceLoadTimerInterval, "Device Load Timer", this);

	if (m_deviceLoadTimer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create device load timer" << m_servicePort;
		return false;
	}

	m_nullRecordGenerationTimer = m_socketMan.createTimer(nullRecordGenerationTimerInterval, "Null Record Generation Timer", this);

	if (m_nullRecordGenerationTimer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create null record generation timer" << m_servicePort;
		return false;
	}

	m_FDCheckTimer = m_socketMan.createTimer(FDCheckTimerInterval, "FD Check Timer", this);

	if (m_FDCheckTimer == nullptr)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to create FD check timer" << m_servicePort;
		return false;
	}

	return true;
}


//*************************************************************************************************
void DataRecorderService::enterRunLoop()
{
	m_socketMan.run();
}


//*************************************************************************************************
void DataRecorderService::OnConnect(ServerSocket* server, ClientSocket* client)
{
	BOOST_LOG_TRIVIAL(info) << "Client connected; FD: " << client->getSocketFD()
					<< ", remoteIP:port: " << client->getRemoteIP() << ":" << client->getRemoteClientPort();

	//Ideally, authentication happens on connect
	//But since there is no such application level protocol  between server and devices, check device ID in each record
}


//*************************************************************************************************
void DataRecorderService::OnDisconnect(ServerSocket* server, ClientSocket* client)
{
	BOOST_LOG_TRIVIAL(info) << "Client disconnected; FD: " << client->getSocketFD()
					<< ", remoteIP:port: " << client->getRemoteIP() << ":" << client->getRemoteClientPort();

	m_lastActiveTimestamp.erase(client->getSocketFD());
}


//*************************************************************************************************
void DataRecorderService::OnData(ServerSocket* server, ClientSocket* client, std::string message)
{
	//Check if special message and take action


	//Amend sender IP and received time
	message = message + ", " + client->getRemoteIP();
	message = message + ", " + getCurrentDatetime();

	if(m_forwardingClient == nullptr || !m_forwardingClient->sendData(message)){
        if(m_forwardingClient != nullptr){
            int forward_fd = m_forwardingClient->getSocketFD();
            m_socketMan.closeClientSocket(forward_fd);
        };
        m_forwardingClient = m_socketMan.createClient((char *) m_configHandler.getConfig("ForwardIP").c_str(),(char *) m_configHandler.getConfig("ForwardPort").c_str(),m_forwarder);
    }

	int clientFD = client->getSocketFD();
	m_lastActiveTimestamp[clientFD] = getTimestamp();	//Update last active timestamp

	BOOST_LOG_TRIVIAL(debug) << "--------------------------------------------------------------------------------------------- \n";
	BOOST_LOG_TRIVIAL(trace) << "Client FD: " << clientFD << "\tData: " << message;

	int result = m_dataStorage->validateAndWriteRecord(message, m_ackContent);

	//Write record cache, update cache and null entry delete cache to database if thresholds are reached
	m_dataStorage->flushCaches();

	if (result == 1)
	{
		//Send ACK to device
		std::string data = "SERVER:" + std::to_string(m_ackContent.first) + "," + std::to_string(m_ackContent.second) + "\r\n";
		client->sendData(data);
	}
	else if (result == -1)	//Unknown device; disconnect
	{
		m_socketMan.closeClientSocket(clientFD);
		m_lastActiveTimestamp.erase(clientFD);
		++m_rejectionCount;
	}

}


//*************************************************************************************************
void DataRecorderService::OnTimer(Timer* timer)
{
	BOOST_LOG_TRIVIAL(info) << "------------------------------------------------ \n";

	if (timer == m_heartbeatTimer)
	{
		BOOST_LOG_TRIVIAL(info) << "Heartbeat timer fired";
		BOOST_LOG_TRIVIAL(info) << "Number of currently connected devices: " << m_lastActiveTimestamp.size();
		dumpServiceInformation();
	}
	else if (timer == m_cacheFlushTimer)
	{
		BOOST_LOG_TRIVIAL(info) << "Cache flush timer fired";
		m_dataStorage->flushCaches(true);
	}
	else if (timer == m_FDCheckTimer)
	{
		BOOST_LOG_TRIVIAL(info) << "FD check timer fired";
		removeInactiveDevices();
	}
	else if (timer == m_deviceLoadTimer)
	{
		BOOST_LOG_TRIVIAL(info) << "Device load timer fired";
		m_dataStorage->initializeDevices(true);
	}
	else if (timer == m_nullRecordGenerationTimer)
	{
		BOOST_LOG_TRIVIAL(info) << "Null record generation timer fired";
		m_dataStorage->FlushAllOutOfOrderRecordsWithNulls();
	}
}


//*************************************************************************************************
void DataRecorderService::removeInactiveDevices()
{
	BOOST_LOG_TRIVIAL(debug) << "Removing following inactive file descriptors (devices)";

	auto iter = m_lastActiveTimestamp.begin();

	while (iter != m_lastActiveTimestamp.end())
	{
		int clientFD = iter->first;
		unsigned long difference = (unsigned long)getTimestamp() - iter->second;

		if (difference > m_deviceInactiveTimeThreshold)
		{
			m_socketMan.closeClientSocket(clientFD);
			m_lastActiveTimestamp.erase(iter++);
			BOOST_LOG_TRIVIAL(debug) << "inactive FD: " << clientFD;
		}
		else
		{
			++iter;
		}
	}
}


//*************************************************************************************************
void DataRecorderService::setDataStorage(DataStorage* dataStorage)
{
	m_dataStorage = dataStorage;
}


//*************************************************************************************************
std::string DataRecorderService::getCurrentDatetime()
{
	time_t t = time(0);   //Get time now
	struct tm * now = localtime(&t);

	std::string year = std::to_string(now->tm_year + 1900);
	std::string month =  std::to_string(now->tm_mon + 1);
	std::string date = std::to_string(now->tm_mday);
	std::string hour = std::to_string(now->tm_hour);
	std::string minute = std::to_string(now->tm_min);
	std::string second = std::to_string(now->tm_sec);

	return year + "/" + month + "/" + date + " " + hour + ":" + minute + ":" + second;
}


//*************************************************************************************************
unsigned long DataRecorderService::getTimestamp()
{
	struct timespec timeSpec;
	clock_gettime(CLOCK_MONOTONIC, &timeSpec);
	return (unsigned long)timeSpec.tv_sec;
}


//*************************************************************************************************
void DataRecorderService::dumpServiceInformation()
{
	const char* filename = "data_recorder_service_information_dump.txt";
	std::ofstream fileStream(filename, std::ios::app);

	if (!fileStream.is_open())
	{
		std::cout << "DataRecorderService::dumpServiceInformation --> cannot open file: " << filename << std::endl;
		return;
	}

	fileStream << "\n\n--------------------------------------------------------------------------" << std::endl;
	fileStream << "Dumping data recorder service information at " << getCurrentDatetime() << '\n' << std::endl;

	fileStream << "------------- Information from class DataRecorderService -------------\n" << std::endl;
	fileStream << "m_rejectionCount = " << m_rejectionCount << '\n' << std::endl;

	fileStream << "### Map m_lastActiveTimestamp" << std::endl;
	for (auto& entry: m_lastActiveTimestamp)
	{
		fileStream << "FD = " << entry.first << ", last active timestamp = " << entry.second << std::endl;
	}
	fileStream << std::endl;

	m_dataStorage->dumpDataStorageInformation(fileStream);

	fileStream.close();
}
