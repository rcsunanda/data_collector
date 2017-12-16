#include <unistd.h>
#include <cstring>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iostream>

#include <DummyEproDevice.h>
#include <ConfigurationHandler.h>

//*************************************************************************************************
bool DummyEproDevice::initialize(int overriddenMsgInterval)
{

	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	//Remote IP, port
	strncpy(m_remoteServerIP, configHandler.getConfig("ServiceIP").c_str(), sizeof(m_remoteServerIP));
	strncpy(m_remoteServerPort, configHandler.getConfig("ServicePort").c_str(), sizeof(m_remoteServerPort));

	//Message delimiter and interval
	m_msgTerminationCharacter = configHandler.getTerminationCharacter();

	m_socketMan.setMsgTerminationCharacter(m_msgTerminationCharacter);

	m_fieldDelimiter = ',';

	m_deviceID = std::stoi(configHandler.getConfig("DummyDeviceID"));
	m_msgIntervalUs = std::stoi(configHandler.getConfig("MessageInterval_us"));
	m_counterPosition = std::stoi(configHandler.getConfig("CounterRecordPosition"));
	m_deviceIDPosition = std::stoi(configHandler.getConfig("DeviceIDRecordPosition"));

	if (overriddenMsgInterval > -1)
		m_msgIntervalUs = overriddenMsgInterval;

	//Data 
	std::string fieldTypesString = configHandler.getConfig("RecordFieldTypes");
	m_fieldTypesVec = splitString(fieldTypesString, ',');

	m_fieldCount = m_fieldTypesVec.size();

	std::cout << "Field count: " << m_fieldCount << std::endl;

	for (auto& entry: m_fieldTypesVec)
		std::cout << entry << '-';

	std::cout << std::endl;

	m_eProClient = m_socketMan.createClient(m_remoteServerIP, m_remoteServerPort, this);

	if (m_eProClient == nullptr)
	{
		std::cout << "Unable to connect to remote data recorder service, "
						<< m_remoteServerIP << ": " << m_remoteServerPort << std::endl;
		return false;
	}

	std::string msgSequenceStr = configHandler.getConfig("MessageSequence");
	std::vector<std::string> stringVec = splitString(msgSequenceStr, ',');

	for (std::string seqStr: stringVec)
		m_msgSequence.push_back(std::stoi(seqStr));

	return true;
}


//*************************************************************************************************
void DummyEproDevice::enterRunLoop()
{
	m_socketMan.run();
}


//*************************************************************************************************
void DummyEproDevice::pumpMessageContinous()
{
	std::cout << "Pumping messages" << std::endl;
	std::cout << "Message count: ";

	int intMessage = 0;
	double doubleMsg = 0.0;
	int stringCounter = 0;

	int msgCount = 0;
	int msgsPerSecond = ceil(1e6/m_msgIntervalUs);

	while(true)
	{
		std::string message;

		//Fill message fields with values
		for (int i = 0; i < m_fieldCount; ++i)
		{
			if (i != 0)
				message += m_fieldDelimiter;

			if (i == m_counterPosition)
			{
				message += std::to_string(msgCount);
				continue;
			}

			if (i == m_deviceIDPosition)
			{
				message += std::to_string(m_deviceID);
				continue;
			}

			if (m_fieldTypesVec[i] == "float" || m_fieldTypesVec[i] == "double")
			{
				doubleMsg += 0.01;
				message += std::to_string(doubleMsg);
			}
			else if (m_fieldTypesVec[i] == "int")
			{
				message += std::to_string(++intMessage);
			}
			else if (m_fieldTypesVec[i] == "char")
			{
				message += std::to_string(10);
			}
			else if (m_fieldTypesVec[i] == "varchar")
			{
				message += "StringMsg_" + std::to_string(++stringCounter);
			}
			else if (m_fieldTypesVec[i] == "datetime")
			{
				message += getCurrentDatetime();
			}
		}
		
		message += m_msgTerminationCharacter;

		bool sendStatus = m_eProClient->sendData(message);
		if (sendStatus == false)
		{
			std::cout << "Failed to send message: " << message << std::endl; //For debugging
			break;
		}

		usleep(m_msgIntervalUs);
		++msgCount;

		
		//std::cout << "msgCount = " << msgCount << ", msgsPerSecond = " << msgsPerSecond << std::endl; //Debug

		if (msgCount % msgsPerSecond == 0)	//Print every second (to minimize competition to to std::cout)
		{
			std::cout << msgCount << ".." << std::flush;
		}

		std::cout << message << std::flush; //For debugging

	}
	std::cout << std::endl << "Disconnected from server, cannot pump messages" << std::endl;
}


//*************************************************************************************************
void DummyEproDevice::pumpMessageSequence()
{
	std::cout << "Pumping messages" << std::endl;
	std::cout << "Message count: ";

	int intMessage = 0;
	double doubleMsg = 0.0;
	int stringCounter = 0;

	int msgCount = 0;
	int msgsPerSecond = ceil(1e6/m_msgIntervalUs);

	for (int msgCounter: m_msgSequence)
	{
		std::string message;

		//Fill message fields with values
		for (int i = 0; i < m_fieldCount; ++i)
		{
			if (i != 0)
				message += m_fieldDelimiter;

			if (i == m_counterPosition)
			{
				message += std::to_string(msgCounter);
				continue;
			}

			if (i == m_deviceIDPosition)
			{
				message += std::to_string(m_deviceID);
				continue;
			}

			if (m_fieldTypesVec[i] == "float" || m_fieldTypesVec[i] == "double")
			{
				doubleMsg += 0.01;
				message += std::to_string(doubleMsg);
			}
			else if (m_fieldTypesVec[i] == "int")
			{
				message += std::to_string(++intMessage);
			}
			else if (m_fieldTypesVec[i] == "char")
			{
				message += std::to_string(10);
			}
			else if (m_fieldTypesVec[i] == "varchar")
			{
				message += "StringMsg_" + std::to_string(++stringCounter);
			}
			else if (m_fieldTypesVec[i] == "datetime")
			{
				message += getCurrentDatetime();
			}
		}

		message += m_msgTerminationCharacter;

		bool sendStatus = m_eProClient->sendData(message);
		if (sendStatus == false)
		{
			std::cout << "Failed to send message: " << message << std::endl; //For debugging
			break;
		}

		usleep(m_msgIntervalUs);
		++msgCount;


		//std::cout << "msgCount = " << msgCount << ", msgsPerSecond = " << msgsPerSecond << std::endl; //Debug

		if (msgCount % msgsPerSecond == 0)	//Print every second (to minimize competition to to std::cout)
		{
			//std::cout << msgCounter << ".." << std::flush;
		}

		std::cout << message << std::flush; //For debugging

	}

	std::cout << std::endl << "Finished pumping messages" << std::endl;
}


//*************************************************************************************************
void DummyEproDevice::pumpMessageCustom()
{
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();
	
	std::string message =configHandler.getConfig("CustomMessageToSend");
	message += m_msgTerminationCharacter;

	std::cout << "Sending the following custom message:" << std::endl;
	std::cout << message << std::endl;


	bool sendStatus = m_eProClient->sendData(message);
	if (sendStatus == false)
	{
		std::cout << "Failed to send message: " << message << std::endl; //For debugging
	}

	std::cout << std::endl << "Finished sending custom message" << std::endl;
}


//*************************************************************************************************
void DummyEproDevice::OnDisconnect(ClientSocket* client)
{
	std::cout << "Device disconnected from server" << std::endl;
	std::cout << "ClientSocket's FD: " << client->getSocketFD() << std::endl;
}


//*************************************************************************************************
void DummyEproDevice::OnData(ClientSocket* client, std::string message)
{
	std::cout << "\t\t" << message << std::endl;
}


//*************************************************************************************************
std::vector<std::string> DummyEproDevice::splitString(std::string input, char delimeter)
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


//*************************************************************************************************
std::string DummyEproDevice::getCurrentDatetime()
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
