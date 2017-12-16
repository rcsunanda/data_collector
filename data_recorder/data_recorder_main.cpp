//Standard C++ headers
#include <string>

//Project headers
#include <ConfigurationHandler.h>
#include <DataStorage.h>
#include <DataRecorderService.h>
#include <Logger.h>

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		BOOST_LOG_TRIVIAL(error) << "Usage: data_recorder <config_filename>";
		return 10;
	}
	
	//Load configurations from file
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();
	if (configHandler.loadConfigurations(argv[1]) == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error loading configurations; exiting ePro data recording program";
		return 10;
	}

	if (configHandler.verifyConfigurations() == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error verifying configurations; exiting ePro data recording program";
		return 10;
	}

	//Initialize logger
	int logLevel, rotationSizeMB, maxLogFileCount;
	try
	{
		logLevel = std::stoi(configHandler.getConfig("LogLevel"));
		rotationSizeMB = std::stoi(configHandler.getConfig("LogFileRotationSize"));
		maxLogFileCount = std::stoi(configHandler.getConfig("MaxLogFileCount"));
	}
	catch (std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Exception thrown by std::stoi() in main() when reading integer configs";
		BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
		return 10;
	}
	std::string logFilenamePrefix = configHandler.getConfig("LogFilenamePrefix");

	initializeLog(logLevel, rotationSizeMB, maxLogFileCount, logFilenamePrefix);

	BOOST_LOG_TRIVIAL(info) << "******************************************************************";
	BOOST_LOG_TRIVIAL(info) << "ePro data recording program started";

	BOOST_LOG_TRIVIAL(info) << "======================================================================";
	BOOST_LOG_TRIVIAL(info) << "Following configurations were loaded successfully";
	configHandler.printConfigurations();
	BOOST_LOG_TRIVIAL(info) << "======================================================================";


	//Initialize storage
	DataStorage dataStorage;
	if (dataStorage.initialize() == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error initializing storage media; exiting ePro data recording program";
		return 10;
	}
	

	//Initialize data recorder service
	DataRecorderService dataRecorder;
	if (dataRecorder.initialize() == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error initializing data recorder service; exiting ePro data recording program";
		return 10;
	}
	dataRecorder.setDataStorage(&dataStorage);

	//Run data recorder service
	dataRecorder.enterRunLoop();

	BOOST_LOG_TRIVIAL(warning) << "Exiting ePro data recording program (which shouldn't happen!)";

	return 0;
}
