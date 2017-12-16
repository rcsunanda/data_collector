//This program tests connectivity issues in the data_recorder service by periodically attempting to connect to it and recording the results

//Compile with the following command
//g++ -std=c++11 connection_test_program_main.cpp -o connection_test_program

//Run in background and direct output to a file as follows
//	./connection_test_program [IP] [port] [no_of_attempts] > file.txt &

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> //inet_ntop
#include <unistd.h>

#include <cstring>	//memset
#include <iostream>
#include <string>

//Functions are implemented below main
int connectToServer(const char* remoteServerIP, const char* remoteServerPort);	//returns socketFD
bool closeConnection(int socketFD);
bool sendData(int socketFD, std::string data);



//*************************************************************************************************
int main(int argc, char** argv)
{
	int attemptCount = 100;
	int successCount = 0, failCount = 0;
	const char* remoteServerIP = "localhost";
	const char* remoteServerPort = "5000";

	if (argc < 4)
	{
		std::cout << "Usage: ./connection_test_program <remote_server_IP> <remote_server_port> <no_of_connection_attempts>" << std::endl;
		return -1;
	}
	else
	{
		remoteServerIP = argv[1];
		remoteServerPort = argv[2];
		attemptCount = atoi(argv[3]);
	}

	std::cout << "Values used; remote IP: " << remoteServerIP << ", remote port: " << remoteServerPort << ", attempt count: " << attemptCount << std::endl;

	std::cout << "Starting connection_test_program" << std::endl;

	for (int i = 0; i < attemptCount; ++i)
	{
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << "attempt number: " << i+1 << " out of " << attemptCount << std::endl;
		
		int socketFD = connectToServer(remoteServerIP, remoteServerPort);
		//sleep(1);
		bool sendStatus = sendData(socketFD, "dummy data string");
		bool closeStatus = closeConnection(socketFD);
		
		std::string result;
		if (socketFD > 0 && sendStatus && closeStatus)
		{
			result = "successful";
			++successCount;
		}
		else
		{
			result = "failure";
			++failCount;
		}

		std::cout << "attempt result: " << result << std::endl;

		sleep(1);
	}

	std::cout << "\n***********************************************" << std::endl;
	std::cout << "Total number of attempts: " << attemptCount << std::endl;
	std::cout << "Successful number of attempts: " << successCount << std::endl;
	std::cout << "Successful percentage: " << successCount*100/attemptCount << "%" << std::endl;
	std::cout << "Failed number of attempts: " << failCount << std::endl;
	std::cout << "Failed percentage: " << failCount*100/attemptCount << "%" << std::endl;

	return 0;
}


//*************************************************************************************************
int connectToServer(const char* remoteServerIP, const char* remoteServerPort)
{
	struct addrinfo addr,*info;
	memset(&addr, 0, sizeof(addrinfo));
	addr.ai_family=AF_UNSPEC;
	addr.ai_socktype=SOCK_STREAM;
	addr.ai_flags=AI_PASSIVE;

	if (getaddrinfo(remoteServerIP, remoteServerPort, &addr, &info) != 0)
	{
		std::cout << "Unable to get address info" << std::endl;
		return -1;
	}

	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD == -1)
	{
		std::cout << "Unable to create socket" << std::endl;
		return -1;
	}

	if (::connect(socketFD, info->ai_addr, info->ai_addrlen) == -1)
	{
		std::cout << "Unable to connect to server: " << remoteServerIP << ":" << remoteServerPort << std::endl;
		return -1;
	}

	freeaddrinfo(info);
	return socketFD;
}


//*************************************************************************************************
bool closeConnection(int socketFD)
{
	if (close(socketFD) == -1)
	{
		std::cout << "Unable to close socket FD: " << socketFD << std::endl;
		std::cout << "errno: " << errno << ", error string: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}


bool sendData(int socketFD, std::string data)
{
	if (::send(socketFD, data.c_str(), data.size(), 0) != (int)data.size())
	{
		std::cout << "Unable to send data on FD: " << socketFD << std::endl;
		std::cout << "errno: " << errno << ", error string: " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}
