#include <fstream>

#include <ConfigurationHandler.h>
#include <Logger.h>


//*************************************************************************************************
ConfigurationHandler::ConfigurationHandler()
{
	//Load escape sequences
	m_escapeCharacterMap["\\n"] = '\n';
	m_escapeCharacterMap["\\t"] = '\t';
	m_escapeCharacterMap["\\0"] = '\0';
	m_escapeCharacterMap["\\n\\r"] = '\r';
	m_escapeCharacterMap["\\r\\n"] = '\n';
}


//*************************************************************************************************
bool ConfigurationHandler::loadConfigurations(std::string filename)
{
	std::ifstream fileStream(filename.c_str());

	std::string line;

	if (fileStream.is_open())
	{
		while (std::getline(fileStream, line))
		{
			std::string key, value;

			size_t delimeterPos = line.find_first_of('=');

			if (delimeterPos == std::string::npos)
				continue; //Equal sign not found (invalid config line)

			key = line.substr(0, delimeterPos);
			key.erase(0, key.find_first_not_of(' '));	//Left trim
			key.erase(key.find_last_not_of(' ') + 1);		//Right trim

			value = line.substr(delimeterPos + 1);
			value.erase(0, value.find_first_not_of(' '));	//Left trim
			value.erase(value.find_last_not_of(' ') + 1);	//Right trim

			if (key.empty() || value.empty())
				continue;

			if (key[0] == '#')
				continue;	//'#' is the comment symbol for the configuration file

			m_configMap[key] = value;
		}

		return true;
	}
	else
	{
		return false;
	}
}


//*************************************************************************************************
bool ConfigurationHandler::verifyConfigurations()
{
	//Check for required configs

	if (m_configMap.count("MySQLServer") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'MySQLServer' is missing";
		return false;
	}
	if (m_configMap.count("Username") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'Username' is missing";
		return false;
	}
	if (m_configMap.count("DatabaseName") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DatabaseName' is missing";
		return false;
	}
	if (m_configMap.count("TableName") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'TableName' is missing";
		return false;
	}
	if (m_configMap.count("PrimaryKeyColumnNameInMainTable") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'PrimaryKeyColumnNameInMainTable' is missing";
		return false;
	}
	if (m_configMap.count("DeviceIDColumnNameInMainTable") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DeviceIDColumnNameInMainTable' is missing";
		return false;
	}
	if (m_configMap.count("RecordCounterColumnNameInMainTable") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'RecordCounterColumnNameInMainTable' is missing";
		return false;
	}
	if (m_configMap.count("DateTimeColumnNameInMainTable") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DateTimeColumnNameInMainTable' is missing";
		return false;
	}
	if (m_configMap.count("DBTableColumnNames") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DBTableColumnNames' is missing";
		return false;
	}
	if (m_configMap.count("DBTableColumnTypes") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DBTableColumnTypes' is missing";
		return false;
	}
	if (m_configMap.count("DeviceRecordPositions") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DeviceRecordPositions' is missing";
		return false;
	}
	if (m_configMap.count("DataRecordType") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DataRecordType' is missing";
		return false;
	}
	if (m_configMap.count("BinaryDataSize") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'BinaryDataSize' is missing";
		return false;
	}
	if (m_configMap.count("DevicesTableName") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DevicesTableName' is missing";
		return false;
	}
	if (m_configMap.count("DeviceIDColumnName") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DeviceIDColumnName' is missing";
		return false;
	}
	if (m_configMap.count("DeviceIDRecordPosition") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'DeviceIDRecordPosition' is missing";
		return false;
	}
	if (m_configMap.count("CounterRecordPosition") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'CounterRecordPosition' is missing";
		return false;
	}
	if (m_configMap.count("NullRecordsTableName") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecordsTableName' is missing";
		return false;
	}
	if (m_configMap.count("NullRecordsTablePrimaryKeyColumn") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecordsTablePrimaryKeyColumn' is missing";
		return false;
	}
	if (m_configMap.count("NullRecDeviceIDColumn") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecDeviceIDColumn' is missing";
		return false;
	}
	if (m_configMap.count("NullRecCounterColumn") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecCounterColumn' is missing";
		return false;
	}
	if (m_configMap.count("NullRecInsertedPrimaryKeyColumn") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecInsertedPrimaryKeyColumn' is missing";
		return false;
	}
	if (m_configMap.count("NullRecRequestCountColumn") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'NullRecRequestCountColumn' is missing";
		return false;
	}
	if (m_configMap.count("ServicePort") == 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Required config 'ServicePort' is missing";
		return false;
	}

	//Hard-coded default values for non-required configs if they were not given in the file

	if (m_configMap.count("Password") == 0)
		m_configMap["Password"] = "";

	if (m_configMap.count("CacheWriteThreshold") == 0)
		m_configMap["CacheWriteThreshold"] = "5";

	if (m_configMap.count("CacheSizeHardLimit") == 0)
		m_configMap["CacheSizeHardLimit"] = "100";

	if (m_configMap.count("NullWriteThreshold") == 0)
		m_configMap["NullWriteThreshold"] = "50";

	if (m_configMap.count("MaxNullRecordCountPerDevice") == 0)
		m_configMap["MaxNullRecordCountPerDevice"] = "100";

	if (m_configMap.count("UpdateCacheThreshold") == 0)
		m_configMap["UpdateCacheThreshold"] = "5";

	if (m_configMap.count("NullEntryDeleteCacheThreshold") == 0)
		m_configMap["NullEntryDeleteCacheThreshold"] = "10";

	if (m_configMap.count("MaxNullRequestCount") == 0)
		m_configMap["MaxNullRequestCount"] = "100";
	
	if (m_configMap.count("FilenamePrefix") == 0)
		m_configMap["OutOfOrderRecordsHardLimit"] = "eProDataStore";

	if (m_configMap.count("LogLevel") == 0)
		m_configMap["LogLevel"] = "3";

	if (m_configMap.count("LogFilenamePrefix") == 0)
		m_configMap["LogFilenamePrefix"] = "eProLog";

	if (m_configMap.count("LogFileRotationSize") == 0)
		m_configMap["LogFileRotationSize"] = "10";

	if (m_configMap.count("MaxLogFileCount") == 0)
		m_configMap["MaxLogFileCount"] = "10";

	if (m_configMap.count("ReceiveBufferSize") == 0)
		m_configMap["ReceiveBufferSize"] = "1024";

	if (m_configMap.count("BufferedMessageHardLimit") == 0)
		m_configMap["BufferedMessageHardLimit"] = "2048";	//for about 20 messages (of size 128 bytes)

	if (m_configMap.count("RecordTerminationCharacter") == 0)
		m_configMap["RecordTerminationCharacter"] = "\n\r";

	if (m_configMap.count("HeartbeatTimerInterval") == 0)
		m_configMap["HeartbeatTimerInterval"] = "180";

	if (m_configMap.count("CacheFlushTimerInterval") == 0)
		m_configMap["CacheFlushTimerInterval"] = "300";

	if (m_configMap.count("DeviceLoadTimerInterval") == 0)
		m_configMap["DeviceLoadTimerInterval"] = "43200";	//12 hours

	if (m_configMap.count("NullRecordGenerationTimerInterval") == 0)
		m_configMap["NullRecordGenerationTimerInterval"] = "86400";	//24 hours

	if (m_configMap.count("FDCheckTimerInterval") == 0)
		m_configMap["FDCheckTimerInterval"] = "800";

	if (m_configMap.count("DeviceInactiveTimeThreshold") == 0)
		m_configMap["DeviceInactiveTimeThreshold"] = "720";	//12 minutes
	
	//Convert msg termination character to char

	std::string& terminationString = m_configMap["RecordTerminationCharacter"];

	if (terminationString.size() == 1)
	{
		m_msgTerminationCharacter = terminationString[0];
	}
	else if (m_escapeCharacterMap.count(terminationString) != 0)
	{
		m_msgTerminationCharacter = m_escapeCharacterMap[terminationString];
	}
	else
	{
		m_msgTerminationCharacter = '\n';
	}

	return true;
}


//*************************************************************************************************
void ConfigurationHandler::printConfigurations()
{
	for (auto& entry: m_configMap)
	{
		if (entry.first == "Password")	//Do not print a password in a log file!
			continue;

		BOOST_LOG_TRIVIAL(info) << entry.first << " = " << entry.second;
	}
}


//*************************************************************************************************
std::string ConfigurationHandler::getConfig(std::string configName)
{
	return m_configMap[configName];
}

//*************************************************************************************************
char ConfigurationHandler::getTerminationCharacter()
{
	return m_msgTerminationCharacter;
}


