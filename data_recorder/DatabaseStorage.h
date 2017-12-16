#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <set>

#include <NullEntry.h>

//MySQL Connector/C++ headers
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/exception.h>
#include <cppconn/warning.h>

/*
This class manages MySQL database I/O
*/
class DatabaseStorage
{
public:
	DatabaseStorage() {}
	~DatabaseStorage() {}

	bool initialize(std::string server, std::string user, std::string password, std::string database, std::string table,
		std::string primaryKeyColumn, std::string recordCounterColumn, std::string deviceIDColumn, std::string dateTimeColumn,
		int columnCount, std::vector<std::string> columnNames, std::vector<std::string> columnTypes, std::vector<int> recordPositions, 
		std::string nullRecordsTable, std::string nullRecTablePrimaryKeyColumn, std::string nullRecInsertedPrimaryKeyColumn, 
		std::string nullRecDeviceIDColumn, std::string nullRecRecordCounterColumn, std::string nullRecRequestCountColumn, int nullEntriesMaxCount);

	bool writeRecordBatch(const std::vector< std::vector<std::string> >& recordBatch);

	bool insertNullRecords(int deviceID, long start, long end, std::vector<NullEntry>& insertedRecordInfoVec);

	bool insertEntriesToNullTable(std::vector<NullEntry>& insertedRecordInfoVec);

	bool updateRecordBatch(const std::unordered_map< long, std::vector<std::string> >& recordBatch);

	bool deleteNullEntryBatch(const std::vector<long>& nullEntryBatch);

	bool loadEntriesFromNullTable(std::set<int>& deviceSet, 
										std::unordered_map< int, std::map<long, NullEntry> >& deviceNullRecordKeys);

	bool getDeviceIDs(std::string tableName, std::string deviceIDColumnName, std::unordered_map<int, long>& deviceLastCounterMap, bool isReinitialize = false);

	bool getInitialNullRecordInfo(const std::unordered_map<int, long>& validDevicesMap, 
							std::unordered_map< int, std::map<long, NullEntry> >& deviceNullRecordKeys);

private:
	//Helper functions
	int splitString(std::string input, char delimeter, std::vector<std::string>& result);

	//Parameters
	std::string m_mySqlServer;
	std::string m_username;
	std::string m_password;
	std::string m_database;
	std::string m_table;	//Main data table
	std::string m_primaryKeyColumn;
	std::string m_recordCounterColumn;
	std::string m_deviceIDColumn;
	std::string m_dateTimeColumn;

	std::string m_nullRecordsTable;
	std::string m_nullRecTablePrimaryKeyColumn;
	std::string m_nullRecInsertedPrimaryKeyColumn;
	std::string m_nullRecDeviceIDColumn;
	std::string m_nullRecRecordCounterColumn;
	std::string m_nullRecRequestCountColumn;

	//For connectivity to MySQL server & database
	sql::Driver* m_driver;
	std::unique_ptr<sql::Connection> m_dbConnection; //unique_ptr for exception handling

	//SQL query stubs for generating frequently occuring queries
	int m_columnCount;
	std::string m_mainInsertQuery;
	std::string m_nullUpdateQueryBeginning;
	std::string m_nullUpdateQueryEnding;
	std::string m_nullInsertQuery;
	std::string m_nullTableEntryInsertQuery;
	std::string m_nullTableEntryDeleteQuery;
	std::string m_nullTableEntryLoadQuery;

	int m_nullEntriesMaxCount; //max. no. of null entries per device to keep in memory

	std::vector<std::string> m_columnNamesVec;
	std::vector<std::string> m_columnTypesVec;
	std::vector<int> m_recordPositionsVec;
};
