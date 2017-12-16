#include <exception>
#include <sstream>
#include <DatabaseStorage.h>
#include <Logger.h>


//*************************************************************************************************
bool DatabaseStorage::initialize(std::string server, std::string user, std::string password, std::string database, std::string table,
		std::string primaryKeyColumn, std::string recordCounterColumn, std::string deviceIDColumn, std::string dateTimeColumn,
		int columnCount, std::vector<std::string> columnNames, std::vector<std::string> columnTypes, std::vector<int> recordPositions,
		std::string nullRecordsTable, std::string nullRecTablePrimaryKeyColumn, std::string nullRecInsertedPrimaryKeyColumn,
		std::string nullRecDeviceIDColumn, std::string nullRecRecordCounterColumn, std::string nullRecRequestCountColumn, int nullEntriesMaxCount)
{
	//Set parameters
	m_mySqlServer = server;
	m_username = user;
	m_password = password;
	m_database = database;
	m_table = table;
	m_primaryKeyColumn = primaryKeyColumn;
	m_recordCounterColumn = recordCounterColumn;
	m_deviceIDColumn = deviceIDColumn;
	m_dateTimeColumn = dateTimeColumn;

	m_nullRecordsTable = nullRecordsTable;
	m_nullRecTablePrimaryKeyColumn = nullRecTablePrimaryKeyColumn;
	m_nullRecInsertedPrimaryKeyColumn = nullRecInsertedPrimaryKeyColumn;
	m_nullRecDeviceIDColumn = nullRecDeviceIDColumn;
	m_nullRecRecordCounterColumn = nullRecRecordCounterColumn;
	m_nullRecRequestCountColumn = nullRecRequestCountColumn;

	m_nullEntriesMaxCount = nullEntriesMaxCount;

	m_columnCount = columnCount;
	m_columnNamesVec = columnNames;
	m_columnTypesVec = columnTypes;
	m_recordPositionsVec = recordPositions;

	//Prepare first part of insert query based on above record structure data
	std::string fields("");

	for (int i = 0; i < m_columnCount; ++i)
	{
		if (i != 0)
			fields += ", ";

		fields += m_columnNamesVec[i];
	}

	m_mainInsertQuery = "INSERT INTO " + m_table + " (" +  fields + ") VALUES " ;	//Main table

	m_nullInsertQuery = "INSERT INTO " + m_table + " (" + m_recordCounterColumn + "," + m_deviceIDColumn + "," + m_dateTimeColumn + ") VALUES ";	//Main table

	m_nullUpdateQueryBeginning = "INSERT INTO " + m_table + " (" + m_primaryKeyColumn + ", " + fields + ") VALUES " ;	//Main table

	m_nullUpdateQueryEnding = ") ON DUPLICATE KEY UPDATE ";

	m_nullTableEntryDeleteQuery = "DELETE FROM " + m_nullRecordsTable + " WHERE " + m_nullRecInsertedPrimaryKeyColumn + " IN (";	//Null records table

	m_nullTableEntryInsertQuery = "INSERT INTO " + m_nullRecordsTable + " (" + m_nullRecDeviceIDColumn
						+ "," + m_nullRecRecordCounterColumn + "," + m_nullRecInsertedPrimaryKeyColumn + ") VALUES ";	//Null records table

	m_nullTableEntryLoadQuery = "SELECT * FROM " + m_nullRecordsTable + " WHERE " + m_nullRecDeviceIDColumn + "=";	//Null records table



	for (int i = 0; i < m_columnCount; ++i)
	{
		m_nullUpdateQueryEnding += m_columnNamesVec[i] + "=VALUES(" + m_columnNamesVec[i];

		if (i != m_columnCount - 1)
		{
			m_nullUpdateQueryEnding += "), " ;
		}
		else
		{
			m_nullUpdateQueryEnding += ");" ;
		}
	}

	BOOST_LOG_TRIVIAL(info) << " ";
	BOOST_LOG_TRIVIAL(info) << "First part of insert query: " << m_mainInsertQuery;
	BOOST_LOG_TRIVIAL(info) << " ";

	//Connect to MySQL server and select database
	try
	{
		BOOST_LOG_TRIVIAL(info) << "Connecting to MySQL server: " << server;

		m_driver = get_driver_instance();
		m_dbConnection = std::unique_ptr<sql::Connection>
							(m_driver->connect(m_mySqlServer, m_username, m_password));

		BOOST_LOG_TRIVIAL(info) << "Connected to MySQL server successfully";

		BOOST_LOG_TRIVIAL(info) << "Selecting database: " << database;
		m_dbConnection->setSchema(m_database);
		BOOST_LOG_TRIVIAL(info) << "Database selected successfully";

		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open connection to MySQL server";
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when opening connection to MySQL";
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::writeRecordBatch(const std::vector< std::vector<std::string> >& recordBatch)
{
	int recordCount = recordBatch.size();

	if (recordCount == 0)
		return true;

	std::string batchInsertQuery = m_mainInsertQuery + ' ';

	for (int j = 0; j < recordCount; ++j)
	{
		batchInsertQuery += '(';

		const std::vector<std::string>& record = recordBatch[j];

		for (int i = 0; i < m_columnCount; ++i)
		{
			if (i > 0)
				batchInsertQuery += ",";

			if (record[m_recordPositionsVec[i]] == "NAN" || record[m_recordPositionsVec[i]] == "NULL" || record[m_recordPositionsVec[i]] == "INF" || record[m_recordPositionsVec[i]] == "OVF")
			{
				batchInsertQuery += "NULL";
				continue;
			}

			if (m_columnTypesVec[i] == "varchar" || m_columnTypesVec[i] == "datetime")	//single quotes around these types
			{
				batchInsertQuery += '\'';
				batchInsertQuery += record[m_recordPositionsVec[i]];
				batchInsertQuery += '\'';
			}
			else
			{
				batchInsertQuery += record[m_recordPositionsVec[i]];
			}
		}

		if (j != recordCount - 1)
		{
			batchInsertQuery += "), " ;
		}
		else
		{
			batchInsertQuery += ");" ;
		}
	}

	BOOST_LOG_TRIVIAL(trace) << "Batch insert query: " << batchInsertQuery;

	try
	{
		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		int numAffectedRows = statement->executeUpdate(batchInsertQuery);

		//If execution comes to this point (no exception was thrown), the query was successfully executed
		//We assume that records were actually inserted

		BOOST_LOG_TRIVIAL(debug) << "record batch size: " << recordCount << ", number of inserted rows: " << numAffectedRows;
		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to insert batch of records into table " << m_table << " with following query: ";
		BOOST_LOG_TRIVIAL(error) << batchInsertQuery;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when inserting batch of records into table: " << m_table;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::insertNullRecords(int deviceID, long start, long end, std::vector<NullEntry>& insertedRecordInfoVec)
{
	int insertedCount = 0;

	BOOST_LOG_TRIVIAL(debug) << "Inserting new null entries to table: " << m_nullRecordsTable;
	BOOST_LOG_TRIVIAL(trace) << "Following null entries are inserted";

	for (int SDcounter = start; SDcounter < end; ++SDcounter)
	{
		std::string nullInsertQuery = m_nullInsertQuery + "(" + std::to_string(SDcounter) + ", " +
														std::to_string(deviceID) + ",'" + "0000/00/00 00:00:00" + "');" ;

		try
		{
			std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
			statement->executeUpdate(nullInsertQuery);

			//If execution comes to this point (no exception was thrown), the query was successfully executed
			//We assume that record was actually inserted

			//Get inserted record's primary key value
			std::unique_ptr<sql::ResultSet> resultSet(statement->executeQuery("SELECT LAST_INSERT_ID() AS primary_key;"));
			resultSet->next();
			long insertedPrimaryKey = resultSet->getUInt("primary_key");

			insertedRecordInfoVec.push_back(NullEntry{deviceID, SDcounter, insertedPrimaryKey, 0});
			++insertedCount;

//			BOOST_LOG_TRIVIAL(trace) << "primaryKey: " << insertedPrimaryKey << ", SDCounter: " << SDcounter << ", deviceID: " << deviceID;
		}
		catch (sql::SQLException &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to insert NULL record into table " << m_table << " with following one of following queries: ";
			BOOST_LOG_TRIVIAL(error) << nullInsertQuery;
			BOOST_LOG_TRIVIAL(error) << "SELECT LAST_INSERT_ID();";
			BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
			return false;
		}
		catch (std::exception &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed when inserting NUL records into table: " << m_table;
			BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
			return false;
		}
	}

	BOOST_LOG_TRIVIAL(debug) << "Generated NULL records inserted to table " << m_table << ", SDCounters: [" << start << "," << end - 1 << "]" << ", no. of NULL records: " << insertedCount;
	return true;
}


//************************************************************************************************
bool DatabaseStorage::insertEntriesToNullTable(std::vector<NullEntry>& insertedRecordInfoVec)
{
	std::string insertQuery = m_nullTableEntryInsertQuery;

	int numEntries = insertedRecordInfoVec.size();

	for (int i = 0; i < numEntries; ++i)
	{
		NullEntry& entry = insertedRecordInfoVec[i];

		insertQuery += "(";
		insertQuery += std::to_string(entry.m_deviceID) + "," + std::to_string(entry.m_SDCounter) +
						"," + std::to_string(entry.m_recordInsertedPrimaryKey) + ")";

		if (i == numEntries - 1)
		{
			insertQuery += ";";
		}
		else
		{
			insertQuery += ",";
		}
	}

	BOOST_LOG_TRIVIAL(trace) << "Entries to null records table insert query: " << insertQuery;

	try
	{
		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		int numAffectedRows = statement->executeUpdate(insertQuery);

		//If execution comes to this point (no exception was thrown), the query was successfully executed
		//We assume that records were actually inserted

		BOOST_LOG_TRIVIAL(debug) << "Entries inserted to table: " << m_nullRecordsTable << ", null entry batch size: "
															<< numEntries << ", number of inserted rows: " << numAffectedRows;
		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to insert null entries into table " << m_nullRecordsTable << " with following query: ";
		BOOST_LOG_TRIVIAL(error) << insertQuery;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when inserting null entries into table: " << m_nullRecordsTable;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::updateRecordBatch(const std::unordered_map< long, std::vector<std::string> >& recordBatch)
{
	int recordCount = recordBatch.size();

	if (recordCount == 0)
		return true;

	std::string batchUpdateQuery = m_nullUpdateQueryBeginning;

	int count = 0;	//To keep track of the number of records in map
	for (auto iter = recordBatch.begin(); iter != recordBatch.end(); ++iter)
	{
		batchUpdateQuery += '(';

		long primaryKey = iter->first;
		const std::vector<std::string>& record = iter->second;

		for (int i = -1; i < m_columnCount; ++i)	//Starts from -1 to accommodate the additional primary key column
		{
			if (i == -1)	//Primary key
			{
				batchUpdateQuery += std::to_string(primaryKey);
				continue;
			}

			if (i > -1)
				batchUpdateQuery += ",";

			if (record[m_recordPositionsVec[i]] == "NAN")
			{
				batchUpdateQuery += "NULL";
				continue;
			}

			if (m_columnTypesVec[i] == "varchar" || m_columnTypesVec[i] == "datetime")	//single quotes around these types
			{
				batchUpdateQuery += '\'';
				batchUpdateQuery += record[m_recordPositionsVec[i]];
				batchUpdateQuery += '\'';
			}
			else
			{
				batchUpdateQuery += record[m_recordPositionsVec[i]];
			}
		}

		if (count != recordCount - 1)
		{
			batchUpdateQuery += "), " ;
		}

		++count;
	}

	batchUpdateQuery += m_nullUpdateQueryEnding;

	BOOST_LOG_TRIVIAL(trace) << "Batch update query: " << batchUpdateQuery;

	try
	{
		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		int numAffectedRows = statement->executeUpdate(batchUpdateQuery);

		//If execution comes to this point (no exception was thrown), the query was successfully executed
		//We assume that records were actually updated

		BOOST_LOG_TRIVIAL(debug) << "Record batch size: " << recordCount << ", number of updated rows: " << numAffectedRows;
		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to update batch of previously-null records in main table " << m_table << " with following query: ";
		BOOST_LOG_TRIVIAL(error) << batchUpdateQuery;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when update batch of previously-null records in main table: " << m_table;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::deleteNullEntryBatch(const std::vector<long>& nullEntryBatch)
{
	int recordCount = nullEntryBatch.size();

	if (recordCount == 0)
		return true;

	std::string batchDeleteQuery = m_nullTableEntryDeleteQuery;

	int count = 0;	//To keep track of the number of records in map

	for (long insertedPrimaryKey: nullEntryBatch)
	{
		batchDeleteQuery += std::to_string(insertedPrimaryKey);

		if (count != recordCount - 1)
			batchDeleteQuery += "," ;

		++count;
	}

	batchDeleteQuery += ");";

	BOOST_LOG_TRIVIAL(trace) << "Batch delete query: " << batchDeleteQuery;

	try
	{
		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		int numAffectedRows = statement->executeUpdate(batchDeleteQuery);

		//If execution comes to this point (no exception was thrown), the query was successfully executed
		//We assume that records were actually deleted

		BOOST_LOG_TRIVIAL(debug) << "record batch size: " << recordCount << ", number of deleted rows: " << numAffectedRows;
		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to delete batch of null entries from table " << m_nullRecordsTable << " with following query: ";
		BOOST_LOG_TRIVIAL(error) << batchDeleteQuery;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when deleting batch of null entries from table: " << m_nullRecordsTable;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::loadEntriesFromNullTable(std::set<int>& deviceSet,
					std::unordered_map< int, std::map<long, NullEntry> >& deviceNullRecordKeys)
{
	if (deviceSet.size() == 0)
		return true;

	BOOST_LOG_TRIVIAL(debug) << "Reloading null entries for following devices to in-memory map from table: " << m_nullRecordsTable;

	for (int deviceID: deviceSet)
	{
		//key=sd_counter, value=vector (vector[0]=primary_key, vector[1]=request_count)
		std::map<long, NullEntry>& nullEntryMap = deviceNullRecordKeys[deviceID];

		long loadAmount = m_nullEntriesMaxCount - nullEntryMap.size();

		if (loadAmount <= 0)
			continue;

		long lastInsertedPrimaryKey = 0;	//Default 0 if map is empty

		if (nullEntryMap.size() > 0)
			lastInsertedPrimaryKey = nullEntryMap.rbegin()->second.m_recordInsertedPrimaryKey;

		std::string loadQuery = m_nullTableEntryLoadQuery;
		loadQuery += std::to_string(deviceID);
		loadQuery = loadQuery + " AND " + m_nullRecInsertedPrimaryKeyColumn + ">" + std::to_string(lastInsertedPrimaryKey);
		loadQuery = loadQuery + " ORDER BY " + m_nullRecTablePrimaryKeyColumn + " LIMIT " + std::to_string(loadAmount) + ";";

		BOOST_LOG_TRIVIAL(debug) << "DeviceID: " << deviceID << ", No. of entries to load: " << loadAmount;
		BOOST_LOG_TRIVIAL(trace) << "Load query: " << loadQuery;

		try
		{
			std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
			std::unique_ptr<sql::ResultSet> resultSet(statement->executeQuery(loadQuery));

			while (resultSet->next())
			{
				unsigned int entryPrimaryKey = resultSet->getUInt(m_nullRecTablePrimaryKeyColumn);
				unsigned int SDCounter = resultSet->getUInt(m_nullRecRecordCounterColumn);
				unsigned int insertedPrimaryKey = resultSet->getUInt(m_nullRecInsertedPrimaryKeyColumn);
				unsigned int requestCount = resultSet->getUInt(m_nullRecRequestCountColumn);

				nullEntryMap.emplace(SDCounter, NullEntry(entryPrimaryKey, deviceID, SDCounter, insertedPrimaryKey, requestCount));
			}
		}
		catch (sql::SQLException &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to load null record information from table: " << m_nullRecordsTable;
			BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
			return false;
		}
		catch (std::exception &e)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed when loading null record information from table: " << m_nullRecordsTable;
			BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
			return false;
		}
	} //End for

	return true;
}


//************************************************************************************************
bool DatabaseStorage::getDeviceIDs(std::string tableName, std::string deviceIDColumnName, std::unordered_map<int, long>& deviceLastCounterMap, bool isReinitialize /*= false*/)
{
	try
	{
		BOOST_LOG_TRIVIAL(info) << "Reading device IDs from table: " << tableName << ", column: " << deviceIDColumnName;

		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		std::unique_ptr<sql::ResultSet> resultSet(statement->executeQuery("SELECT " + deviceIDColumnName + " FROM " + tableName));

		while (resultSet->next())
		{
			int deviceID = resultSet->getUInt(deviceIDColumnName);

			if (deviceLastCounterMap.count(deviceID) == 0)	//If not already in the map (new device being added)
			{
				deviceLastCounterMap[deviceID] = 0;	//Add with zero last counter
			}
		}

		BOOST_LOG_TRIVIAL(info) << "Device IDs read from table " << tableName << " successfully";

		if (isReinitialize)	//Return without retrieving max from main table
			return true;

		BOOST_LOG_TRIVIAL(info) << "Reading last counter of devices from table: " << m_table;

		std::string maxColumnName = "max_" + m_recordCounterColumn;
		std::string maxQuery = "SELECT " + m_deviceIDColumn + ", MAX(" + m_recordCounterColumn + ") AS " + maxColumnName +
																		" FROM " + m_table + " GROUP BY " + m_deviceIDColumn;

		std::unique_ptr<sql::Statement> lastCounterStatement(m_dbConnection->createStatement());
		std::unique_ptr<sql::ResultSet> lastCounterResultSet(statement->executeQuery(maxQuery));

		while (lastCounterResultSet->next())
		{
			int deviceID = lastCounterResultSet->getUInt(m_deviceIDColumn);

			if (deviceLastCounterMap.count(deviceID) != 0)	//Device was loaded from devices table
			{
				deviceLastCounterMap[deviceID] = lastCounterResultSet->getUInt(maxColumnName);	//Add with zero last counter
			}
		}

		BOOST_LOG_TRIVIAL(info) << "Last counter of each device read from table " << m_table << " successfully";

		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to get device IDs and last counter from tables: " << tableName << ", " << m_table;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to get device IDs and last counter from tables: " << tableName << ", " << m_table;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}


//************************************************************************************************
bool DatabaseStorage::getInitialNullRecordInfo(const std::unordered_map<int, long>& validDevicesMap,
									std::unordered_map< int, std::map<long, NullEntry> >& deviceNullRecordKeys)
{
	std::string selectQuery = "SELECT * FROM " + m_nullRecordsTable + " ORDER BY " + m_nullRecTablePrimaryKeyColumn + " ASC;";
	try
	{
		BOOST_LOG_TRIVIAL(info) << "Retrieving null record information from table: " << m_nullRecordsTable;

		std::unique_ptr<sql::Statement> statement(m_dbConnection->createStatement());
		std::unique_ptr<sql::ResultSet> resultSet(statement->executeQuery(selectQuery));

		while (resultSet->next())
		{
			int deviceID = resultSet->getUInt(m_nullRecDeviceIDColumn);

			if (validDevicesMap.count(deviceID) == 0)
				continue;	//Skip this device

			//Ensure that the device is a valid device (in the devices table)

			std::map<long, NullEntry>& nullEntryMap = deviceNullRecordKeys[deviceID];

			if (nullEntryMap.size() < m_nullEntriesMaxCount)
			{
				unsigned int entryPrimaryKey = resultSet->getUInt(m_nullRecTablePrimaryKeyColumn);
				unsigned int SDCounter = resultSet->getUInt(m_nullRecRecordCounterColumn);
				unsigned int insertedPrimaryKey = resultSet->getUInt(m_nullRecInsertedPrimaryKeyColumn);
				unsigned int requestCount = resultSet->getUInt(m_nullRecRequestCountColumn);

				nullEntryMap.emplace(SDCounter, NullEntry(entryPrimaryKey, deviceID, SDCounter, insertedPrimaryKey, requestCount));
			}
		}

		BOOST_LOG_TRIVIAL(info) << "Null record information read from table " << m_nullRecordsTable << " successfully";

		return true;
	}
	catch (sql::SQLException &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to get null record information from table: " << m_nullRecordsTable;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed when getting null record information from table: " << m_nullRecordsTable;
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return false;
	}
}
