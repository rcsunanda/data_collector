#pragma once

#include <vector>
#include <string>
#include <fstream>

/*
This class manages file I/O
*/
class FileBasedStorage
{
public:
	FileBasedStorage();
	~FileBasedStorage();

	bool initialize(std::string filename);

	bool writeRecord(std::string& recordString);
	void writeRecordBatch(const std::vector< std::vector<std::string> >& recordBatch);

private:
	std::string convertVectorToString(const std::vector<std::string>& record);

	std::ofstream m_fileStream;

	std::string m_currentFilename;
	int m_currentFileRecordCount;
	int m_totalFileRecordCount;
};
