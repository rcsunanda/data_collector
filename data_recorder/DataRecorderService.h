#pragma once

#include <utility>
#include <map>
#include <ConfigurationHandler.h>
#include <SocketCommunication.h>

class DataStorage;
class ConfigurationHandler;
class ConfigurationHandler;
class Forwarder;
class DataRecorderService: public SocketCallback
{
public:

	DataRecorderService();
	~DataRecorderService() {}

	bool initialize();
	void setDataStorage(DataStorage* dataStorage);

	void enterRunLoop();

	//Server side callbacks
	virtual void OnConnect(ServerSocket* server, ClientSocket* client);
	virtual void OnDisconnect(ServerSocket* server, ClientSocket* client);
	virtual void OnData(ServerSocket* server, ClientSocket* client, std::string message);

	//Timer callback
	virtual void OnTimer(Timer* timer);

private:
	std::string getCurrentDatetime();
	unsigned long getTimestamp();
	void dumpServiceInformation();

	void removeInactiveDevices();

	SocketManager m_socketMan;
	ServerSocket* m_dataRecorderServer;
    ConfigurationHandler& m_configHandler = ConfigurationHandler::getInstance();
    ClientSocket* m_forwardingClient;
	Forwarder* m_forwarder;

	Timer* m_heartbeatTimer;
	Timer* m_cacheFlushTimer;
	Timer* m_deviceLoadTimer;
	Timer* m_nullRecordGenerationTimer;
	Timer* m_FDCheckTimer;

	unsigned long m_deviceInactiveTimeThreshold;

	DataStorage* m_dataStorage;

	char m_servicePort[20];

	char m_msgTerminationCharacter;

	std::pair<long, int> m_ackContent;

	//key=FD, value=timestamp
	std::map<int, unsigned long> m_lastActiveTimestamp;

	unsigned long m_rejectionCount;
};
