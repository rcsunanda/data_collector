#include <ctime>
#include <algorithm> //max
#include <sstream>
#include <set>
#include <exception>

#include <DataStorage.h>
#include <ConfigurationHandler.h>
#include <Logger.h>


//*************************************************************************************************
DataStorage::DataStorage():
	m_isDatabaseActive{false},
	m_isFileActive{false},
	m_dbInactiveCount{0},
	m_dbInactiveCountThreshold{20},
	m_failedBatchWriteCount{0},
	m_failedBatchUpdateCount{0},
	m_bactchWriteFailCountThreshold{10},
	m_dbWriteCount{0},
	m_cachedRecordCount{0},
	m_cachedNullUpdateCount{0},
	m_cachedNullEntryDeleteCount{0},
	m_fileWriteCount{0},
	m_deviceIDPosition{0}
{
}


//*************************************************************************************************
bool DataStorage::initialize()
{
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	try
	{
		m_nullWriteThreshold = std::stoi(configHandler.getConfig("NullWriteThreshold"));
		m_cacheWriteThreshold = std::stoi(configHandler.getConfig("CacheWriteThreshold"));
		m_cacheSizeHardLimit = std::stoi(configHandler.getConfig("CacheSizeHardLimit"));
		m_updateCacheThreshold = std::stoi(configHandler.getConfig("UpdateCacheThreshold"));
		m_nullEntryDeleteCacheThreshold = std::stoi(configHandler.getConfig("NullEntryDeleteCacheThreshold"));

		m_maxNullRecordRequestCount = std::stoi(configHandler.getConfig("MaxNullRequestCount"));

		m_deviceIDPosition = std::stoi(configHandler.getConfig("DeviceIDRecordPosition"));
		m_counterPosition = std::stoi(configHandler.getConfig("CounterRecordPosition"));

		m_maxNullCountPerDevice = std::stoi(configHandler.getConfig("MaxNullRecordCountPerDevice"));
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Exception thrown by std::stoi() in DataStorage::initialize() when reading integer configs";
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	
	//Get storage parameters
	std::string mySqlServer = configHandler.getConfig("MySQLServer");
	std::string username = configHandler.getConfig("Username");
	std::string password = configHandler.getConfig("Password");
	std::string database = configHandler.getConfig("DatabaseName");
	std::string table = configHandler.getConfig("TableName");
	std::string primaryKeyColumn = configHandler.getConfig("PrimaryKeyColumnNameInMainTable");
	std::string recordCounterColumn = configHandler.getConfig("RecordCounterColumnNameInMainTable");
	std::string deviceIDColumnInMainTable = configHandler.getConfig("DeviceIDColumnNameInMainTable");
	std::string dateTimeColumn = configHandler.getConfig("DateTimeColumnNameInMainTable");

	std::string nullRecordsTable = configHandler.getConfig("NullRecordsTableName");
	std::string nullRecTablePrimaryKeyColumn = configHandler.getConfig("NullRecordsTablePrimaryKeyColumn");
	std::string nullRecInsertedPrimaryKeyColumn = configHandler.getConfig("NullRecInsertedPrimaryKeyColumn");
	std::string nullRecDeviceIDColumn = configHandler.getConfig("NullRecDeviceIDColumn");
	std::string nullRecRecordCounterColumn = configHandler.getConfig("NullRecCounterColumn");
	std::string nullRecRequestCountColumn = configHandler.getConfig("NullRecRequestCountColumn");

	
	
	std::string filenamePrefix = configHandler.getConfig("FilenamePrefix");

	//Get current date
	time_t t = time(0);
	struct tm* now = localtime(&t);
	std::string year = std::to_string(now->tm_year + 1900);
	std::string month = std::to_string(now->tm_mon + 1);
	std::string date = std::to_string(now->tm_mday);

	std::string filename = filenamePrefix + "_" + year + "_" + month;
	
	BOOST_LOG_TRIVIAL(info) << "======================================================================";
	BOOST_LOG_TRIVIAL(info) << "=====Initializing data storage media=====";

	//Initialize record structure
	BOOST_LOG_TRIVIAL(info) << "===Initializing record structure===";
	if (initializeRecordStructure() == false)
		return false;

	//Initialize database
	BOOST_LOG_TRIVIAL(info) << "===Initializing database storage===";
	if (m_dbStorage.initialize(mySqlServer, username, password, database, table, primaryKeyColumn, recordCounterColumn, 
							deviceIDColumnInMainTable, dateTimeColumn , m_columnCount, m_columnNamesVec, m_columnTypesVec, 
							m_recordPositionsVec, nullRecordsTable, nullRecTablePrimaryKeyColumn, nullRecInsertedPrimaryKeyColumn, 
							nullRecDeviceIDColumn, nullRecRecordCounterColumn, nullRecRequestCountColumn, m_maxNullCountPerDevice))
	{
		m_isDatabaseActive = true;

		//Initialize device IDs
		BOOST_LOG_TRIVIAL(info) << "===Initializing devices===";
		if (initializeDevices() == false)
			return false;

		//Initialize null-written records information
		BOOST_LOG_TRIVIAL(info) << "===Initializing null-written records information===";
		if (initializeNullRecords() == false)
			return false;
	}

	//Initialize file
	BOOST_LOG_TRIVIAL(info) << "===Initializing file based storage===";
	if (m_fileStorage.initialize(filename))
		m_isFileActive = true;


	//Log and return
	if (m_isDatabaseActive) //Even if file storage fails at startup, it is ok
	{
		BOOST_LOG_TRIVIAL(info) << "=====Storage media initialization successful=====";
		BOOST_LOG_TRIVIAL(info) << "======================================================================";
		return true;
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "=====Storage media initialization failed (both database and file)=====";
		BOOST_LOG_TRIVIAL(info) << "======================================================================";
		return false;
	}
}


//*************************************************************************************************
bool DataStorage::initializeRecordStructure()
{
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	std::string columnNamesString = configHandler.getConfig("DBTableColumnNames");
	std::string columnTypesString = configHandler.getConfig("DBTableColumnTypes");
	std::string recordPositionsString = configHandler.getConfig("DeviceRecordPositions");

	m_columnNamesVec = splitString(columnNamesString, ',');
	m_columnTypesVec = splitString(columnTypesString, ',');

	//Create m_recordPositionsVec
	std::vector<std::string> recordPositionsStringVec = splitString(recordPositionsString, ',');
	m_recordPositionsVec.clear();

	for (auto& position: recordPositionsStringVec)
	{
		try
		{
			m_recordPositionsVec.push_back(std::stoi(position));
		}
		catch (std::exception &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Exception thrown by std::stoi() in DataStorage::initializeRecordStructure() when reading record positions";
			BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
			return false;
		}
	}

	//Find the maximum position (for record verification)
	auto maxIter = max_element(m_recordPositionsVec.begin(), m_recordPositionsVec.end());
	m_largestRecordPosition = *maxIter;

	int nameCount = m_columnNamesVec.size();
	int typeCount = m_columnTypesVec.size();
	int positionCount = m_recordPositionsVec.size();

	if (nameCount != typeCount)
	{
		BOOST_LOG_TRIVIAL(error) << "The number of table column names and column types do not match. Check the configuration file";
		return false;
	}

	if (nameCount != positionCount)
	{
		BOOST_LOG_TRIVIAL(error) << "The number of table column names and device record positions do not match. Check the configuration file";
		return false;
	}

	m_columnCount = nameCount;
	
	BOOST_LOG_TRIVIAL(info) << "Record structure initialized successfully";
	return true;
}


//*************************************************************************************************
bool DataStorage::initializeDevices(bool isReinitialize /*= false*/)
{
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	std::string deviceTableName = configHandler.getConfig("DevicesTableName");
	std::string deviceIDColumnName = configHandler.getConfig("DeviceIDColumnName");	//In devices table
	
	//std::vector<int> deviceIDs;

	if (m_dbStorage.getDeviceIDs(deviceTableName, deviceIDColumnName, m_deviceLastCounterInDBMap, isReinitialize) == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Retrieving device IDs from database failed";
		return false;
	}
	
	BOOST_LOG_TRIVIAL(info) << "Devices loaded from table: " << deviceTableName << " successfully";

	//Debug printing
	BOOST_LOG_TRIVIAL(info) << "======================================================================";
	BOOST_LOG_TRIVIAL(trace) << "===Printing device map===";
	for (auto& entry: m_deviceLastCounterInDBMap)
	{
		BOOST_LOG_TRIVIAL(trace) << entry.first << "--" << entry.second;
	}
	BOOST_LOG_TRIVIAL(info) << "Total number of devices read from device table: " << m_deviceLastCounterInDBMap.size();
	BOOST_LOG_TRIVIAL(info) << "======================================================================";

	return true;
}


//*************************************************************************************************
bool DataStorage::initializeNullRecords()
{
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();

	std::string nullRecordsTable = configHandler.getConfig("NullRecordsTableName");
	
	if (m_dbStorage.getInitialNullRecordInfo(m_deviceLastCounterInDBMap, m_deviceNullRecordKeys) == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Retrieving null record information from database failed";
		return false;
	}

	BOOST_LOG_TRIVIAL(info) << "Null record information loaded from table: " << nullRecordsTable << " successfully";

	//Debug printing
	BOOST_LOG_TRIVIAL(info) << "======================================================================";
	BOOST_LOG_TRIVIAL(trace) << "===Printing null record information map===";
	int totalCount = 0;
	for (auto& outEntry: m_deviceNullRecordKeys)
	{
		BOOST_LOG_TRIVIAL(trace) << "**** device ID = " << outEntry.first << " ****";
		for (auto& inEntry: m_deviceNullRecordKeys[outEntry.first])
		{
			BOOST_LOG_TRIVIAL(trace) << "\tSDCounter: " << inEntry.first << ", PrimaryKey: " 
					<< inEntry.second.m_recordInsertedPrimaryKey << ", RequestCount: " << inEntry.second.m_requestCount;
			++totalCount;
		}
	}
	BOOST_LOG_TRIVIAL(info) << "Total number of null records retrieved: " << totalCount;
	BOOST_LOG_TRIVIAL(info) << "======================================================================";

	return true;
}


//*************************************************************************************************
//Return values
//-1 = unknown device so disconnect
//0 = do not send ACK
//1 = send ACK
int DataStorage::validateAndWriteRecord(std::string recordString, std::pair<long, int>& ackContent)
{
	m_fieldValues = splitString(recordString, ',');
	int valueCount = m_fieldValues.size();

	if (m_deviceIDPosition >= valueCount)	//Can't retrieve device ID
		return -1;	//Disconnect device

	int deviceID;
	try
	{
		deviceID = std::stoi(m_fieldValues[m_deviceIDPosition]);
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Exception thrown by std::stoi() when reading device ID from record";
		BOOST_LOG_TRIVIAL(warning) << "Error: " << e.what();
		return -1;	//Disconnect device
	}

	//Verify that the record is coming from a known device
	if (m_deviceLastCounterInDBMap.count(deviceID) == 0)
	{
		BOOST_LOG_TRIVIAL(warning) << "Data received by unknown device. Device ID = " << deviceID;
		return -1;	//Disconnect device
	}

	long lastCounter = m_deviceLastCounterInDBMap[deviceID];
	long currentCounter;
	try
	{
		currentCounter = std::stol(m_fieldValues[m_counterPosition]);
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Exception thrown by std::stol() when reading SD counter from record";
		BOOST_LOG_TRIVIAL(warning) << "Error: " << e.what();
		return -1;	//Disconnect device
	}

	int outOfOrderCount = m_deviceOutOfOrderStore[deviceID].size();
	BOOST_LOG_TRIVIAL(debug) << "deviceID: " << deviceID << ", lastCounter: " << lastCounter << ", currentCounter: " << currentCounter;
	BOOST_LOG_TRIVIAL(debug) << "in-order record cache size: " << m_cachedRecordCount << ", out-of order record count for device: " << outOfOrderCount <<
				", null update cache size: " << m_cachedNullUpdateCount << ", null entry delete cache size: " << m_cachedNullEntryDeleteCount;

	if (lastCounter == 0) //This is the first record the server is receiving from this device after it was added to the devices table
	{
		BOOST_LOG_TRIVIAL(info) << "first record the server is receiving from this device after it was added to the devices table, (lastCounter = 0)";
		lastCounter = currentCounter - 1;
	}

	//Validate field count in record
	if (m_largestRecordPosition >= valueCount)
	{
		BOOST_LOG_TRIVIAL(warning) << "The number of fields in input message (" << valueCount << ") is smaller than the minimum required (" << m_largestRecordPosition + 1 << ")";
		ackContent.first = currentCounter;
		ackContent.second = 0;
		return -1;	//Disconnect device
	}
	
	//We can do further validations (eg: whether each field has the correct type, required fields are set...)
	//But it may be costly to do it for every message. We assume that the devices send proper messages

	//*******************************************************************
	//Check whether the received record is maintaining order, or a previous null-written record, and maintain internal state

	std::map< long, std::vector<std::string> >& outOfOrderRecordStore = m_deviceOutOfOrderStore[deviceID];

	if (currentCounter == lastCounter + 1)	//Correct next record
	{
		BOOST_LOG_TRIVIAL(debug) << "Correct next record in-order (currentCounter == lastCounter + 1)";
		
		m_recordCache.push_back(m_fieldValues);
		++m_cachedRecordCount;
		m_deviceLastCounterInDBMap[deviceID] = currentCounter;

		if (outOfOrderRecordStore.size() != 0)	//Check whether the current record filled the gap between in-order and out-of-order records
		{
			long smallestOutOfOrderRecordCounter = outOfOrderRecordStore.begin()->first;

			if (currentCounter == smallestOutOfOrderRecordCounter - 1)	//Gap filled; write consecutive out-of order records to cache
			{
				BOOST_LOG_TRIVIAL(debug) << "Current record filled the gap between in-order and out-of-order records (currentCounter == smallestTempRecordCounter - 1)";

				BOOST_LOG_TRIVIAL(debug) << "Moving the following consecutive out-of-order records to in-order cache: ";

				long tempCounter = currentCounter + 1;

				auto iter = outOfOrderRecordStore.begin();
				while (iter != outOfOrderRecordStore.end())
				{
					if (iter->first == tempCounter)
					{
						m_recordCache.push_back(iter->second);
						++m_cachedRecordCount;

						if (m_cachedRecordCount >= m_cacheWriteThreshold)	//This situation can offer if a large no. of out of order records existed
							writeRecordCache();

						BOOST_LOG_TRIVIAL(debug) << tempCounter;
						++tempCounter;
						++iter;
					}
					else //Write only upto the largest incrementing counter
					{
						break;
					}
				}

				--tempCounter;
				m_deviceLastCounterInDBMap[deviceID] = tempCounter;	//Update last written counter in map

				BOOST_LOG_TRIVIAL(debug) << "Moving finished" ;
				outOfOrderRecordStore.erase(outOfOrderRecordStore.begin(), iter);	//Remove moved record block from out-of-order store
				m_cachedRecordCount = m_recordCache.size();	//Unnecessary; m_cachedRecordCount is already the correct size
			}

			return 0; //Do not send ACK (since this record was sent while there were out-of-order records, this is most likely a requested record; no need to ACK)
		}
		else
		{
			generateACK(deviceID, m_deviceLastCounterInDBMap[deviceID], currentCounter, ackContent);
			return 1;	//Send ACK
		}
	}
	else if (currentCounter > lastCounter + 1)	//Out-of-order record; request missing ones and save current record for writing later
	{
		BOOST_LOG_TRIVIAL(debug) << "Out-of-order record (currentCounter > lastCounter + 1)";

		outOfOrderRecordStore[currentCounter] = m_fieldValues;

		if (outOfOrderRecordStore.size() >= m_nullWriteThreshold)	//Move records to cache with NULL records generated for missing records
			FlushOutOfOrderRecordsWithNulls(deviceID, lastCounter, outOfOrderRecordStore);

		generateACK(deviceID, m_deviceLastCounterInDBMap[deviceID], currentCounter, ackContent);
		return 1;	//Send ACK
	}
	else if (currentCounter <= lastCounter)	//Past record (eg: previous null-written record)
	{
		BOOST_LOG_TRIVIAL(debug) << "Past record (currentCounter <= lastCounter): (this record could be a previously null-written record)";

		//key=sd_counter
		std::map<long, NullEntry>& nullRecordKeys = m_deviceNullRecordKeys[deviceID];

		if (nullRecordKeys.count(currentCounter) != 0)	//Previous null-written record exists; add to update cache
		{
			BOOST_LOG_TRIVIAL(debug) << "Previously null-written record exists for this record; adding it to null-update cache";

			NullEntry& nullEntry = nullRecordKeys[currentCounter];
			long insertedPrimaryKey = nullEntry.m_recordInsertedPrimaryKey;

			m_nullUpdateCache[insertedPrimaryKey] = m_fieldValues;
			++m_cachedNullUpdateCount;
		}
		else	//Previous null-written record does not exist; completely ignore?
		{
			BOOST_LOG_TRIVIAL(debug) << "Previously null-written record does not exist for this record; ignoring it";
		}

		return 0; //Do not send ACK
	}
}


//*************************************************************************************************
void DataStorage::generateACK(int deviceID, long lastCounter, long currentCounter, std::pair<long, int>& ackContent)
{
	//First check whether device has previous null entries and request earliest consecutive range
	
	//This must be done before iterating the nullEntriesMap as received null entries may not have been removed from it
	//The overhead is negligible as in most cases the updateCache will be empty and no DB operation will happen
	updateNullCache();

	//key=sd_counter
	std::map<long, NullEntry>& nullEntriesMap = m_deviceNullRecordKeys[deviceID];

	if (nullEntriesMap.size() !=0 )	//Null entries exist
	{
		auto iter = nullEntriesMap.begin();
		long tempCounter = iter->first;
		long startSDCounter = tempCounter;
		int consecutiveNullEntryCount = 0;
		int deletedCount = 0;

		while (iter != nullEntriesMap.end())
		{
			if (iter->first == tempCounter)
			{
				unsigned int& requestCountRef = iter->second.m_requestCount;
				
				if (requestCountRef >= m_maxNullRecordRequestCount)
				{
					m_nullEntryDeleteCache.push_back(iter->second.m_recordInsertedPrimaryKey);
					nullEntriesMap.erase(iter++);	//Advance iterator with post increment as it is invalidated after erase
					++m_cachedNullEntryDeleteCount;
					++deletedCount;
				}
				else
				{
					++iter;
				}

				++requestCountRef;
				++tempCounter;
				++consecutiveNullEntryCount;
			}
			else //Request only upto the largest incrementing counter
			{
				break;
			}
		}

		ackContent.first = startSDCounter;
		ackContent.second = consecutiveNullEntryCount;

		BOOST_LOG_TRIVIAL(debug) << "ACK generated by examining device's past null entries (earliest consecutive range): SERVER:" << ackContent.first << "," << ackContent.second;

		//It is somewhat inefficient to load new entries here (during ACK generation)
		//But in most cases, a block of null entries will exceed the max request count, so this inefficiency is negligible
		if (deletedCount > 0)	//Reload null entries from null_records table
		{
			BOOST_LOG_TRIVIAL(debug) << "Null entries were removed since max request count was exceeded. Deleted entry count: " << deletedCount;
			std::set<int> tempSetForDevice;
			tempSetForDevice.insert(deviceID);
			flushNullEntryDeleteCache();
			m_dbStorage.loadEntriesFromNullTable(tempSetForDevice, m_deviceNullRecordKeys);
		}

		return;
	}

	//No previous null entries; check gap between in-order cache and out-of-order store

	std::map< long, std::vector<std::string> >& outOfOrderRecordStore = m_deviceOutOfOrderStore[deviceID];

	if (outOfOrderRecordStore.size() != 0)	//Out-of-order records exist
	{
		ackContent.first = lastCounter + 1;	//The +1 is important as device will send records starting from that number
		ackContent.second = outOfOrderRecordStore.begin()->first - lastCounter - 1;
		BOOST_LOG_TRIVIAL(debug) << "ACK generated by gap between in-order cache and out-of-order store: SERVER:" << ackContent.first << "," << ackContent.second;
		return;
	}
	else
	{
		ackContent.first = lastCounter;	//Since we're not requesting any records the +1 is not required here (and would be meaningless)
		ackContent.second = 0;
		BOOST_LOG_TRIVIAL(debug) << "Simple ACK generated as no previous records must be requested: SERVER:" << ackContent.first << "," << ackContent.second;
		return;
	}
	
}


//*************************************************************************************************
bool DataStorage::writeRecordCache()
{
	if (m_recordCache.size() == 0)
		return true;

	BOOST_LOG_TRIVIAL(debug) << "Writing record batch to database, batch size: " << m_recordCache.size();

	if (m_dbStorage.writeRecordBatch(m_recordCache))
	{
		m_recordCache.clear();	//clear the record cache
		m_cachedRecordCount = 0;
		return true;
	}
	
	//Database write failed --> re-initialize after a fail count threshold is reached
	
	//TODO: This checking of database connection and re-initialization can be done on a timer --> lightweight querry or other check $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	++m_failedBatchWriteCount;
	if (m_failedBatchWriteCount > m_bactchWriteFailCountThreshold)
	{
		BOOST_LOG_TRIVIAL(warning) << "Re-initializing database as failed write count threshold reached";
		m_failedBatchWriteCount = 0;
		if (initialize())
			return false;	//False, because writing failed
	}

	//Check whether cache size has reached hard limit and flush to file

	if (m_cachedRecordCount >= m_cacheSizeHardLimit)
	{
		m_fileStorage.writeRecordBatch(m_recordCache);
		m_recordCache.clear();
		m_cachedRecordCount = m_recordCache.size();
	}

	return false;
}


//*************************************************************************************************
bool DataStorage::FlushOutOfOrderRecordsWithNulls(int deviceID, long lastCounter, 
									std::map< long, std::vector<std::string> >& outOfOrderRecordStore)
{
	if (outOfOrderRecordStore.size() == 0)	//Possible when triggered by timer
		return true;

	//Flush cached in-order records first
	if (writeRecordCache() == false)
		return false;

	//Generate NULL records and insert them to main table

	BOOST_LOG_TRIVIAL(debug) << "Generating NULL records and inserting them to main table";

	++lastCounter;
	long smallestOutOfOrderRecordCounter = outOfOrderRecordStore.begin()->first;

	//Vector to hold information about inserted null records
	std::vector<NullEntry> insertedRecordInfoVec;

	if (m_dbStorage.insertNullRecords(deviceID, lastCounter, smallestOutOfOrderRecordCounter, insertedRecordInfoVec) == false)
	{
		if (insertedRecordInfoVec.size() == 0)	//No NULL records were inserted to main table
			return false;
	}

	BOOST_LOG_TRIVIAL(debug) << "Inserting corresponding null entries to null_records table";

	//Insert corresponding null entries to null records table
	if (m_dbStorage.insertEntriesToNullTable(insertedRecordInfoVec) == false)
	{
		//We assume that if the above NULL record insertion to main table was successful, there is no reason for this insertion to fail
		return false;
	}

	//Update in-memory null records information map

	BOOST_LOG_TRIVIAL(debug) << "Updating in-memory null records information map";

	//key=sd_counter
	std::map<long, NullEntry>& deviceNullKeysMap = m_deviceNullRecordKeys[deviceID];

	int size = deviceNullKeysMap.size();

	for (NullEntry& nullEntry: insertedRecordInfoVec)
	{
		if (size >= m_maxNullCountPerDevice)
			break;

		deviceNullKeysMap.emplace(nullEntry.m_SDCounter, std::move(nullEntry));
		++size;
	}

	//Move in-order records in the outOfOrderRecordStore to m_recordCache

	BOOST_LOG_TRIVIAL(debug) << "Moving the following out-of-order records to in-order cache (after writing generated NULL records): ";

	auto iter = outOfOrderRecordStore.begin();
	long tempCounter = iter->first;

	while (iter != outOfOrderRecordStore.end())
	{
		if (iter->first == tempCounter)
		{
			m_recordCache.push_back(iter->second);

			if (m_recordCache.size() >= m_cacheWriteThreshold)	//This situation can offer if a large no. of out of order records existed
				writeRecordCache();

			BOOST_LOG_TRIVIAL(debug) << tempCounter;
			++tempCounter;
			++iter;
		}
		else //Write only up to the largest consecutive counter
		{
			break;
		}
	}

	//Update last written counter
	--tempCounter;
	m_deviceLastCounterInDBMap[deviceID] = tempCounter;

	BOOST_LOG_TRIVIAL(debug) << "Moving finished" ;
	outOfOrderRecordStore.erase(outOfOrderRecordStore.begin(), iter);	//Remove in-order record block
	m_cachedRecordCount = m_recordCache.size();
}


//*************************************************************************************************
bool DataStorage::FlushAllOutOfOrderRecordsWithNulls()
{
	for (auto& device: m_deviceLastCounterInDBMap)
	{
		int deviceID = device.first;
		long lastCounter = device.second;

		std::map< long, std::vector<std::string> >& outOfOrderRecordStore = m_deviceOutOfOrderStore[deviceID];

		FlushOutOfOrderRecordsWithNulls(deviceID, lastCounter, outOfOrderRecordStore);
	}
}


//*************************************************************************************************
bool DataStorage::updateNullCache()
{
	if (m_nullUpdateCache.size() == 0)
		return true;

	if (m_dbStorage.updateRecordBatch(m_nullUpdateCache))
	{
		BOOST_LOG_TRIVIAL(debug) << "Null update cache was written to database, update batch size: " << m_nullUpdateCache.size();
		BOOST_LOG_TRIVIAL(trace) << "Removing following updated null entries from in-memory map and adding them to delete cache";
		
		std::set<int> deletedEntriesDevices;

		for (auto& updatedRecord: m_nullUpdateCache)
		{
			long insertedPrimaryKey = updatedRecord.first;
			int deviceID = std::stoi((updatedRecord.second)[m_deviceIDPosition]);	//We assume stoi will not fail here as they have been verified before this point
			long SDCounter = std::stoi((updatedRecord.second)[m_counterPosition]);

//			BOOST_LOG_TRIVIAL(trace) << "Device ID: " << deviceID << ", SDCounter: " << SDCounter;

			m_nullEntryDeleteCache.push_back(insertedPrimaryKey);	//Add to delete cache

			//Delete written elements from in-memory map (after marking devices that require reloading of null entries from table)

			//key=sd_counter
			std::map<long, NullEntry>& deviceNullKeysMap = m_deviceNullRecordKeys[deviceID];

			//Loading new elements is necessary only if the in-memory map previously had entries up to the allowed limit
			if (deviceNullKeysMap.size() >= m_maxNullCountPerDevice)
				deletedEntriesDevices.insert(deviceID);

			deviceNullKeysMap.erase(SDCounter);
		}

		//Flush delete cache
		//this is needed because all null entries for a device could be deleted and it will be impossible to fine lastInsertedPrimaryKey in loadEntriesFromNullTable
		flushNullEntryDeleteCache();

		//Load new elements up to m_maxNullCountPerDevice for each device in deletedEntriesMap
		m_dbStorage.loadEntriesFromNullTable(deletedEntriesDevices, m_deviceNullRecordKeys);

		m_nullUpdateCache.clear();	//clear the record cache
		m_cachedNullUpdateCount = m_nullUpdateCache.size();
		return true;
		
		
	}

	return false;
}


//*************************************************************************************************
bool DataStorage::flushNullEntryDeleteCache()
{
	if (m_nullEntryDeleteCache.size() == 0)
		return true;

	if (m_dbStorage.deleteNullEntryBatch(m_nullEntryDeleteCache))
	{
		BOOST_LOG_TRIVIAL(debug) << "Null entry delete cache was written to database, delete batch size: " << m_nullEntryDeleteCache.size();
		m_nullEntryDeleteCache.clear();
		m_cachedNullEntryDeleteCount = 0;
		return true;
	}

	return false;
}


//*************************************************************************************************
bool DataStorage::flushCaches(bool timerFired /*= false*/)
{
	bool writeCache = false;
	bool updateCache = false;
	bool deleteCache = false;

	if (m_cachedRecordCount >= m_cacheWriteThreshold || timerFired)
	{
		if (writeRecordCache())
			writeCache = true;
	}

	if (m_cachedNullUpdateCount >= m_updateCacheThreshold || timerFired)
	{
		if (updateNullCache())
			updateCache = true;
	}

	if (m_cachedNullEntryDeleteCount >= m_nullEntryDeleteCacheThreshold || timerFired)
	{
		if (flushNullEntryDeleteCache())
			deleteCache = true;
	}

	return (writeCache && updateCache && deleteCache);
}


//*************************************************************************************************
std::vector<std::string> DataStorage::splitString(std::string input, char delimeter)
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
std::string DataStorage::convertVectorToString(std::vector<std::string>& record)
{
	std::string result;

	for (int i = 0; i < record.size(); ++i)
	{
		if (i > 0)
			result += ",";

		result += record[i];
	}

	return result;
}


//*************************************************************************************************
void DataStorage::dumpDataStorageInformation(std::ofstream& fileStream)
{
	fileStream << "------------- From class DataStorage -------------\n" << std::endl;
	
	fileStream << "### Map m_deviceLastCounterInDBMap" << std::endl;
	for (auto& entry: m_deviceLastCounterInDBMap)
	{
		fileStream << "device ID = " << entry.first << " --> last written counter = " << entry.second << std::endl;
	}
	fileStream << std::endl;


	fileStream << "### Vector m_recordCache (Cache that stores in-order records for batch writing)" << std::endl;
	for (auto& record: m_recordCache)
	{
		fileStream << "device ID = " << record[m_deviceIDPosition] << ", SD counter = " << record[m_counterPosition] << std::endl;
	}
	fileStream << std::endl;


	fileStream << "### Map m_deviceOutOfOrderStore (Temporary store for out of order records)" << std::endl;
	for (auto& entry: m_deviceOutOfOrderStore)
	{
		auto& outOfOrderRecordMap = entry.second;
		fileStream << "device ID = " << entry.first << " --> SD counters: ";

		for (auto& nestedEntry: outOfOrderRecordMap)
			fileStream << nestedEntry.first << ", ";

		fileStream << std::endl;
	}
	fileStream << std::endl;


	fileStream << "### Map m_nullUpdateCache (Cache that stores newly received previous null-written records (for UPDATEs))" << std::endl;
	for (auto& entry: m_nullUpdateCache)
	{
		auto& record = entry.second;
		fileStream << "original primary key = " << entry.first << 
			" --> device ID: " << record[m_deviceIDPosition] << ", SD counter: " << record[m_counterPosition] << std::endl;
	}
	fileStream << std::endl;


	fileStream << "### Vector m_nullEntryDeleteCache (Cache that stores null entries to delete from null_records table)" << std::endl;
	for (long insertedPrimaryKey: m_nullEntryDeleteCache)
	{
		fileStream << "original (inserted) primary key = " << insertedPrimaryKey << ", ";
	}
	fileStream << std::endl;


	fileStream << "### Map m_deviceNullRecordKeys (Null records' primary key map)" << std::endl;
	for (auto& entry: m_deviceNullRecordKeys)
	{
		auto& nullRecordsMap = entry.second;
		fileStream << "device ID = " << entry.first << std::endl;

		for (auto& nestedEntry: nullRecordsMap)
		{
			NullEntry& nullEntry = nestedEntry.second;
			fileStream << '\t' << "SD counter = " << nestedEntry.first << ", inserted primary key = " << nullEntry.m_entryPrimaryKey
								<< ", request count = " << nullEntry.m_requestCount << std::endl;
		}

		fileStream << std::endl;
	}
	fileStream << std::endl;


	fileStream << "### Vector m_recordCache (Cache that stores in-order records for batch writing)" << std::endl;
	for (auto& record: m_recordCache)
	{
		fileStream << "device ID = " << record[m_deviceIDPosition] << ", SD counter = " << record[m_counterPosition] << std::endl;
	}
	fileStream << std::endl;

	fileStream << "### Important scalar variables" << std::endl;
	fileStream << "m_cachedRecordCount = " << m_cachedRecordCount << ", m_recordCache.size() = " << m_recordCache.size() << std::endl;
	fileStream << "m_cachedNullUpdateCount = " << m_cachedNullUpdateCount << ", m_nullUpdateCache.size() = " << m_nullUpdateCache.size() << std::endl;
	fileStream << "m_cachedNullEntryDeleteCount = " << m_cachedNullEntryDeleteCount << ", m_nullEntryDeleteCache.size() = " << m_nullEntryDeleteCache.size() << std::endl;
}


