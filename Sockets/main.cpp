/*
	Winsock program
*/

#include <WinSock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <cstdio>

#include "stringhelpers.h"
#include "cmdargs.h"

#include "network.h"

#pragma comment(lib, "Ws2_32.lib")

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool connectServer(const char* serverAddress, const char* port, SOCKET& listenSocket);
bool testServer(SOCKET socket);
bool disconnectServer(SOCKET socket);

bool connectClient(const char* serverAddress, const char* port, SOCKET& socket);
bool disconnectClient(SOCKET clientSocket);
bool testClient(SOCKET socket);

void getAddressName(addrinfo* info, char* buffer, size_t szbuffer, int flag = NI_NUMERICHOST);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename ... args_t>
void printclient(const char* format, args_t ... args)
{
	//Convert timestamp into hours/minutes/seconds
	time_t stamp = time(0);
	tm t;
	localtime_s(&t, &stamp);
	char timestr[200];
	strftime(timestr, sizeof(timestr), "[%H:%M:%S]", &t);

	printf(((std::string)timestr + " CLIENT: " + format + "\n").c_str(), args...);
}

template<typename ... args_t>
void printserver(const char* format, args_t ... args)
{
	//Convert timestamp into hours/minutes/seconds
	time_t stamp = time(0);
	tm t;
	localtime_s(&t, &stamp);
	char timestr[200];
	strftime(timestr, sizeof(timestr), "[%H:%M:%S]", &t);

	printf(((std::string)timestr + " SERVER: " + format + "\n").c_str(), args...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Entry point
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	CommandLineArgs args(argv, argc);

	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		printf("failed to initialize winsock. error code: %d\n", WSAGetLastError());
		return 1;
	}

	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET serverSocket = INVALID_SOCKET;

	std::string address;
	std::string port("27015");

	if (args.isArgumentTag("address"))
		address = args.getArgumentValue("address");
	if (args.isArgumentTag("port"))
		port = args.getArgumentValue("port");
	
	bool isServer = args.isArgumentTag("server");
	bool isClient = args.isArgumentTag("client");

	printf("Address: %s\n", address.c_str());
	printf("Port: %s\n", port.c_str());

	if (isServer && !isClient)
	{
		//address.c_str()
		if (!connectServer(nullptr, port.c_str(), serverSocket))
			return 1;

		if (!testServer(serverSocket))
			return 1;

		disconnectServer(serverSocket);
	}
	else if (isClient && !isServer)
	{
		if (!connectClient(address.c_str(), port.c_str(), clientSocket))
			return 1;

		if (!testClient(clientSocket))
			return 1;

		disconnectClient(clientSocket);
	}
	else
	{
		printf("Invalid arguments, program must specify either --server/--client as a single argument\n");
	}

	WSACleanup();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Client functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool connectClient(const char* serverAddress, const char* port, SOCKET& clientSocket)
{
	addrinfo info;
	ZeroMemory(&info, sizeof(addrinfo));

	info.ai_family = AF_UNSPEC; //Do not specify a family so IPv4 and IPv6 addresses can be used
	info.ai_socktype = SOCK_STREAM;
	info.ai_protocol = IPPROTO_TCP;
	//info.ai_flags = AI_NUMERICHOST;

	//A structure containing address and port information for the server we want to connect to
	addrinfo* serverAddressInfoList = nullptr;

	//Attempt to get server address information from the name of the server and the port
	if (int result = getaddrinfo(serverAddress, port, &info, &serverAddressInfoList))
	{
		printclient("failed to get address information. WSA error %d", result);
		return false;
	}

	int connectionStatus = 0;

	//Attempt to create socket connections for different addresses
	for (addrinfo* addressInfo = serverAddressInfoList; addressInfo != nullptr; addressInfo = addressInfo->ai_next)
	{
		//Attemp to connect to server using address information
		clientSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);

		if (clientSocket == INVALID_SOCKET)
		{
			printclient("failed to create socket. WSA error %d", WSAGetLastError());
		}

		//Get IP
		CHAR addressString[1024] = {};
		ZeroMemory(&addressString, sizeof(addressString));
		getAddressName(addressInfo, addressString, ARRAYSIZE(addressString));

		printclient("requesting connection to server (%s:%s)...", addressString, port);

		//Try to connect the socket using this address
		if (connectionStatus = connect(clientSocket, addressInfo->ai_addr, addressInfo->ai_addrlen))
		{
			printclient("connection failed (%d). Trying again...", connectionStatus);

			//If the next address in the linked list is null then there are no more connections to try
			if (addressInfo->ai_next == nullptr)
			{
				printclient("no more connections");
				break;
			}
		}
		else
		{
			//If the returned result of connect is 0 then we have a successful connection
			//The loop stops when addressInfo is null
			printclient("connection established");
			addressInfo = nullptr;
			break;
		}
	}

	freeaddrinfo(serverAddressInfoList);

	if (connectionStatus || clientSocket == INVALID_SOCKET)
	{
		printclient("failed to connect socket. WSA error %d", WSAGetLastError());
		closesocket(clientSocket);
		return false;
	}

	return true;
}

bool testClient(SOCKET clientSocket)
{
	if (clientSocket == INVALID_SOCKET)
		return false;

	const int recieveBufferLength = 512;
	char recieveBuffer[recieveBufferLength] = {};
	const char sendBuffer[] = "this is a test";

	//Send some data
	printclient("transmitting string \"%s\"", sendBuffer);
	if (send(clientSocket, sendBuffer, (int)strlen(sendBuffer), 0) == SOCKET_ERROR)
	{
		printclient("send failed: %d", WSAGetLastError());
		closesocket(clientSocket);
		return false;
	}

	//shutdown the connection for sending since no more data will be sent
	//the client can still use the ConnectSocket for receiving data
	if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR)
	{
		printclient("shutdown failed: %d", WSAGetLastError());
		closesocket(clientSocket);
		return 1;
	}

	int recieveResult = 0;
	// Receive data until the server closes the connection
	do
	{
		recieveResult = recv(clientSocket, recieveBuffer, recieveBufferLength, 0);

		if (recieveResult > 0)
		{
			printclient("bytes received: %d", recieveResult);
			printclient("\"%s\"", recieveBuffer);
		}
		else if (recieveResult == 0)
		{
			printclient("connection closed");
		}
		else
		{
			printclient("recieve failed: %d", WSAGetLastError());
		}

	} while (recieveResult > 0);

	return true;
}

bool disconnectClient(SOCKET clientSocket)
{
	// shutdown the send half of the connection since no more data will be sent
	if (shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR)
	{
		printclient("shutdown failed: %d", WSAGetLastError());
		closesocket(clientSocket);
		return false;
	}

	closesocket(clientSocket);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Server functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool connectServer(const char* serverAddress, const char* port, SOCKET& listenSocket)
{
	//listenSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		printserver("failed to create socket. WSA error %d", WSAGetLastError());
		return false;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(std::stoi(port));
	
	//if (bind(listenSocket, addressInfo->ai_addr, addressInfo->ai_addrlen) == SOCKET_ERROR)
	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printserver("bind failed with error: %d", WSAGetLastError());
		closesocket(listenSocket);
		return false;
	}

	//Get and display server ip
	char addressBuffer[1024];
	ZeroMemory(&addressBuffer, sizeof(addressBuffer));
	//getAddressName(addressInfo, addressBuffer, ARRAYSIZE(addressBuffer));
	getnameinfo((const SOCKADDR*)&serverAddr, sizeof(serverAddr), addressBuffer, ARRAYSIZE(addressBuffer), nullptr, 0, AI_NUMERICHOST);
	printserver("socket created and bound (%s:%s)", addressBuffer, port);

	printserver("listening for connections...");

	//Listen for incoming connections
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("SERVER: listen failed with error: %ld", WSAGetLastError());
		closesocket(listenSocket);
		return false;
	}

	return true;
}

bool testServer(SOCKET listenSocket)
{
	if (listenSocket == INVALID_SOCKET)
		return false;
	
	//Stores information about the client address
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	ZeroMemory(&clientAddr, clientAddrLen);

	//Wait for a client connection - creates a new client socket
	SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

	printserver("connections found.");

	if (clientSocket == INVALID_SOCKET)
	{
		printserver("accept failed with error: %d", WSAGetLastError());
		closesocket(listenSocket);
		return false;
	}

	//Get address name of client connection as a string
	const int clientAddrBufferSize = 1024;
	char clientAddrBuffer[clientAddrBufferSize];
	ZeroMemory(clientAddrBuffer, clientAddrBufferSize); 
	if (getnameinfo((const SOCKADDR*)&clientAddr, clientAddrLen, clientAddrBuffer, clientAddrBufferSize, nullptr, 0, NI_NUMERICHOST))//NI_NOFQDN  NI_NUMERICHOST
	{
		printserver("unable to resolve client address string");
	}

	printserver("connection accepted (%s)", clientAddrBuffer);

	//Listen socket no longer needed
	closesocket(listenSocket);

	//Try sending/reading some data
	const int recieveBufferLength = 512;
	char recieveBuffer[recieveBufferLength] = {};

	int recieveResult = 0;
	int sendResult = 0;

	//Send a hello message to the client
	char initialSendMessage[] = "Hello client.";
	if (send(clientSocket, initialSendMessage, strlen(initialSendMessage), 0) == SOCKET_ERROR)
	{
		printserver("send failed: %d", WSAGetLastError());
		closesocket(clientSocket);
		return false;
	}

	do
	{
		//Clear recieve buffer
		//ZeroMemory(&recieveBuffer, sizeof(recieveBuffer));

		recieveResult = recv(clientSocket, recieveBuffer, recieveBufferLength, 0);

		if (recieveResult > 0)
		{
			printserver("bytes received: %d", recieveResult);

			//Print recieved information
			printserver("\"%s\"", recieveBuffer);

			// Echo the buffer back to the sender
			if ((sendResult = send(clientSocket, recieveBuffer, recieveResult, 0)) == SOCKET_ERROR)
			{
				printserver("send failed: %d", WSAGetLastError());
				closesocket(clientSocket);
				return false;
			}

			printserver("bytes sent: %d", sendResult);
		}
		else if (recieveResult == 0)
		{
			printserver("connection closing...");
		}
		else
		{
			printserver("recv failed: %d", WSAGetLastError());
			closesocket(clientSocket);
			return 1;
		}

	} while (recieveResult > 0);

	if (shutdown(clientSocket, SD_BOTH) == SOCKET_ERROR)
	{
		printserver("shutdown failed: %d", WSAGetLastError());
		closesocket(clientSocket);
		return false;
	}

	closesocket(clientSocket);

	printserver("connection closed");

	return true;
}

bool disconnectServer(SOCKET socket)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Obtains address name as a string
void getAddressName(addrinfo* info, char* buffer, size_t szbuffer, int flag)
{
	if (getnameinfo(info->ai_addr, sizeof(SOCKADDR), buffer, szbuffer, nullptr, 0, flag))
	{
		printf("unable to resolve address name\n");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////