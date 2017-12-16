#pragma once

#include <string>
#include <map>

/*
This singleton class loads and provides configuration info to the program
*/
class ConfigurationHandler
{
public:
	static ConfigurationHandler& getInstance()
	{
		static ConfigurationHandler instance;	// Instantiated on first use.
		return instance;
	}
	
	bool loadConfigurations(std::string filename);
	bool verifyConfigurations();
	void printConfigurations();

	std::string getConfig(std::string configName);
	char getTerminationCharacter();

private:
	ConfigurationHandler();

	//Delete copy c'tor and assignment operator
	ConfigurationHandler(ConfigurationHandler const&) = delete;
	void operator=(ConfigurationHandler const&) = delete;

	std::map<std::string, std::string> m_configMap;

	char m_msgTerminationCharacter;
	std::map<std::string, char> m_escapeCharacterMap;
};
