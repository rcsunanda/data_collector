//Standard C++ headers
#include <iostream>
#include <string>
#include <cstdlib>
#include <pthread.h>

//Project headers
//#include <SocketCommunication.h>
#include <DummyEproDevice.h>
#include <ConfigurationHandler.h>
#include <Logger.h>


void* ACKReceiver(void* data)
{
	std::cout << "Started ACKReceiver thread" << std::endl;
	
	DummyEproDevice* dummyDevice = static_cast<DummyEproDevice*>(data);
	dummyDevice->enterRunLoop();

	return NULL;
}

int main(int argc, char* argv[])
{
	int messageInterval = -1;
	int customMsgSequence = 0;

	if (argc < 2)
	{
		std::cout << "Usage: dummy_ePRO <config_filename> [msg_interval(us): override] [custom_msg_sequence_on]" << std::endl;
		return -1;
	}

	if (argc > 2)
	{
		messageInterval = std::stoi(argv[2]);
	}

	if (argc > 3)
	{
		customMsgSequence = std::stoi(argv[3]);
	}

	std::cout << "Dummy ePRO device started" << std::endl;

	//Load configurations from file
	ConfigurationHandler& configHandler = ConfigurationHandler::getInstance();
	if (configHandler.loadConfigurations(argv[1]) == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error loading configurations; exiting...";
		return -1;
	}

	DummyEproDevice* dummyDevice = new DummyEproDevice();

	if (dummyDevice->initialize(messageInterval) == false)
	{
		BOOST_LOG_TRIVIAL(error) << "Error initializing dummy device; exiting...";
		return -1;
	}

	//Create separate thread for receiving ACKS
	pthread_t ackReceiverThread;
	if(pthread_create(&ackReceiverThread, NULL, ACKReceiver, dummyDevice))
	{
		std::cout << "Error creating ACKReceiver thread" << std::endl;
		return -1;
	}

	//Message pumping works in main thread
	if (customMsgSequence == 1)
	{
		dummyDevice->pumpMessageSequence();
	}
	else if (customMsgSequence == 2)
	{
		dummyDevice->pumpMessageCustom();
	}
	else
	{
		dummyDevice->pumpMessageContinous();
	}

	//pthread_join(ackReceiverThread, NULL);

	return 0;
}
