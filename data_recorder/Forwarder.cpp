#include <cstring>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iostream>
#include <ClientSocket.h>
#include <Forwarder.h>



void Forwarder::OnDisconnect(ClientSocket* client)
{
	std::cout << "Device disconnected from server" << std::endl;
	std::cout << "ClientSocket's FD: " << client->getSocketFD() << std::endl;
}


//*************************************************************************************************
void Forwarder::OnData(ClientSocket* client, std::string message)
{
	std::cout << "\t\t" << message << std::endl;
}
