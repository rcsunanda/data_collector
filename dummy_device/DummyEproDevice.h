#pragma once

#include <string>

#include <SocketCommunication.h>

class ConfigurationHandler;

class DummyEproDevice: public SocketCallback
{
public:
	
	DummyEproDevice() {}
	~DummyEproDevice() {}

	bool initialize(int overriddenMsgInterval);
	void enterRunLoop();

	void pumpMessageContinous();

	void pumpMessageSequence();
	
	void pumpMessageCustom();

	//Client side callbacks
	virtual void OnDisconnect(ClientSocket* client);
	virtual void OnData(ClientSocket* client, std::string message);

private:
	//Helper functions
	std::vector<std::string> splitString(std::string input, char delimeter);
	std::string getCurrentDatetime();

	SocketManager m_socketMan;
	ClientSocket* m_eProClient;

	char m_remoteServerIP[20];
	char m_remoteServerPort[20];

	//Message related variables
	std::vector<std::string> m_fieldTypesVec;
	char m_fieldDelimiter;
	char m_msgTerminationCharacter;
	int m_msgIntervalUs;
	int m_fieldCount;
	int m_counterPosition;
	int m_deviceIDPosition;

	int m_deviceID;

	std::vector<int> m_msgSequence;
};
