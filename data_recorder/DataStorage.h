#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <utility>
#include <fstream>	//for dumping service info to file

#include <DatabaseStorage.h>
#include <FileBasedStorage.h>
#include <NullEntry.h>


/*
This class is acts as an interface to the user for a primary and secondary data storage
*/
class DataStorage
{
public:

	DataStorage();
	~DataStorage() {}
	bool initialize();
	bool initializeDevices(bool isReinitialize = false);

	int validateAndWriteRecord(std::string recordString, std::pair<long, int>& ackContent);
	
	//per device (on threashold reached)
	bool FlushOutOfOrderRecordsWithNulls(int deviceID, long lastCounter, std::map< long, std::vector<std::string> >& outOfOrderRecordStore);

	//all devices (on timer)
	bool FlushAllOutOfOrderRecordsWithNulls();

	bool writeRecordCache();
	bool updateNullCache();
	bool flushNullEntryDeleteCache();
	
	bool flushCaches(bool timerFired = false);

	void dumpDataStorageInformation(std::ofstream& fileStream);

private:
	//Helper functions
	std::vector<std::string> splitString(std::string input, char delimeter);
	std::string convertVectorToString(std::vector<std::string>& record);
	std::string getCurrentDatetime();

	bool initializeRecordStructure();
	bool initializeNullRecords();

	void generateACK(int deviceID, long lastCounter, long currentCounter, std::pair<long, int>& ackContent);


	DatabaseStorage m_dbStorage;
	FileBasedStorage m_fileStorage;

	bool m_isDatabaseActive;
	bool m_isFileActive;

	int m_dbInactiveCount;
	int m_dbInactiveCountThreshold;

	int m_failedBatchWriteCount;
	int m_bactchWriteFailCountThreshold;

	int m_failedBatchUpdateCount;
	int m_bactchUpdateFailCountThreshold;

	unsigned long m_dbWriteCount;
	unsigned long m_fileWriteCount;
	
	//For authentication and maintaining order of records
	//Load device list from database on startup and then repeatedly on timer
	//key = deviceID, value = last written record's ID
	std::unordered_map<int, long> m_deviceLastCounterInDBMap;

	//Temporary store for out of order records
	//key = deviceID, value = map of records keyed by record sd_counter
	std::unordered_map< int, std::map< long, std::vector<std::string> > > m_deviceOutOfOrderStore;
	int m_nullWriteThreshold;

	//Null records' primary key map
	//key = deviceID, value = map of null entries
	//nested map's key=sd_counter
	std::unordered_map< int, std::map<long, NullEntry> > m_deviceNullRecordKeys;
	int m_maxNullCountPerDevice;

	//Cache to store in-order records for batch writing
	std::vector< std::vector<std::string> > m_recordCache;
	int m_cachedRecordCount;
	int m_cacheWriteThreshold;
	int m_cacheSizeHardLimit;	//maximum allowed in cache

	//Cache to store newly received previous null-written records (for UPDATEs)
	//key = original primary key, value = record
	std::unordered_map< long, std::vector<std::string> > m_nullUpdateCache;
	int m_cachedNullUpdateCount;
	int m_updateCacheThreshold;
	int m_updateCacheSizeHardLimit;	//maximum allowed in cache

	//Cache to store null entries to delete from null_records table
	//value=nullRecInsertedPrimaryKey
	std::vector<long> m_nullEntryDeleteCache;
	int m_cachedNullEntryDeleteCount;
	int m_nullEntryDeleteCacheThreshold;

	//Position of device ID in a record (0 based index)
	int m_deviceIDPosition;

	//Position of record counter in a record (0 based index)
	int m_counterPosition;

	//Minimum no. of requests to send before removing a null entry (from in-memory map and null_records table)
	int m_maxNullRecordRequestCount;

	//For parsing and validating incoming messages
	//These variables are also set at initialization
	int m_columnCount;
	int m_largestRecordPosition;

	std::vector<std::string> m_columnNamesVec;
	std::vector<std::string> m_columnTypesVec;
	std::vector<int> m_recordPositionsVec;

	//Field values in each message
	std::vector<std::string> m_fieldValues;
	
};
