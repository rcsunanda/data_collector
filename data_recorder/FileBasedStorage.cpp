#include <FileBasedStorage.h>
#include <Logger.h>


//*************************************************************************************************
FileBasedStorage::FileBasedStorage():
	m_totalFileRecordCount{0},
	m_currentFileRecordCount{0}
{
}


//*************************************************************************************************
FileBasedStorage::~FileBasedStorage()
{
	m_fileStream.close();
}


//*************************************************************************************************
bool FileBasedStorage::initialize(std::string filename)
{
	//Close currently used file
	if (m_fileStream.is_open())
	{
		m_fileStream.close();
		BOOST_LOG_TRIVIAL(info) << "Closed currently open file: " << m_currentFilename;
	}

	//Initialize new file
	m_currentFileRecordCount = 0;

	BOOST_LOG_TRIVIAL(info) << "Opening file :" << filename << "...";
	m_fileStream.open(filename.c_str(), std::ofstream::app);

	if (m_fileStream.is_open())
	{
		m_currentFilename = filename;
		BOOST_LOG_TRIVIAL(info) << "File opened successfully";
		return true;
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "File to open file";
		return false;
	}
}


//*************************************************************************************************
bool FileBasedStorage::writeRecord(std::string& recordString)
{
	m_fileStream << recordString;
	m_fileStream.flush(); //For synchronization
	BOOST_LOG_TRIVIAL(debug) << "File write successful";
	return true;
}

//*************************************************************************************************
void FileBasedStorage::writeRecordBatch(const std::vector< std::vector<std::string> >& recordBatch)
{
	
	for (auto& record: recordBatch)
	{
		std::string recordString = convertVectorToString(record);
		m_fileStream << recordString << '\n'; 
	}
	//m_fileStream << recordBatchString;
	m_fileStream.flush(); //For synchronization
	BOOST_LOG_TRIVIAL(info) << "Record batch was written to file (no. of records: " << recordBatch.size() << ")";
}


//*************************************************************************************************
std::string FileBasedStorage::convertVectorToString(const std::vector<std::string>& record)
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
