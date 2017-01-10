/*
	Server system
*/

#include "winsock.h"
#include "common.h"

#include "netclient.h"
#include "netserver.h"

#include <functional>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Server implementation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_CLIENT_CONTEXTS 8

struct ClientContext
{
	SOCKET clientSocket = INVALID_SOCKET;
	thread clientThread;
	char recieveBuffer[RECV_BUFFER_SIZE] = {0};
};

struct NetworkServer::Impl
{
	NetworkServer* m_server = nullptr;

	SOCKET m_listenSocket = INVALID_SOCKET;
	thread m_listenThread;

	ClientContext m_clients[MAX_CLIENT_CONTEXTS];
	ClientId m_clientIdCounter = 0;

	bool m_success = false;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Initialize server listening socket on a specific port
	bool serverInit(const char* port)
	{
		//listenSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
		m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (m_listenSocket == INVALID_SOCKET)
		{
			printserver("failed to create socket. WSA error %d", WSAGetLastError());
			return false;
		}

		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(std::stoi(port));

		//if (bind(listenSocket, addressInfo->ai_addr, addressInfo->ai_addrlen) == SOCKET_ERROR)
		if (::bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			printserver("bind() failed with error: %d", WSAGetLastError());
			closesocket(m_listenSocket);
			return false;
		}

		//Get and display server ip
		char addressBuffer[1024];
		ZeroMemory(&addressBuffer, sizeof(addressBuffer));
		//getAddressName(addressInfo, addressBuffer, ARRAYSIZE(addressBuffer));
		getnameinfo((const SOCKADDR*)&serverAddr, sizeof(serverAddr), addressBuffer, ARRAYSIZE(addressBuffer), nullptr, 0, AI_NUMERICHOST);
		printserver("socket created and bound (%s:%s)", addressBuffer, port);

		printserver("listening for connections...");

		//Set socket to a listening state
		if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			printf("SERVER: listen() failed with error: %ld", WSAGetLastError());
			closesocket(m_listenSocket);
			return false;
		}
		
		return true;
	}

	//Wait for client connections
	bool serverListen()
	{
		//Stores information about the client address
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);
		ZeroMemory(&clientAddr, clientAddrLen);

		while (true)
		{
			//Wait for a client connection - creates a new client socket
			SOCKET s = accept(m_listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

			const ClientId id = m_clientIdCounter;
			m_clientIdCounter++;

			printserver("connections found.");

			if (s == INVALID_SOCKET)
			{
				printserver("accept failed with error: %d", WSAGetLastError());
				return false;
			}

			m_clients[id].clientSocket = s;

			//Get address name of client connection as a string
			const int clientAddrBufferSize = 1024;
			char clientAddrBuffer[clientAddrBufferSize];
			ZeroMemory(clientAddrBuffer, clientAddrBufferSize);
			if (getnameinfo((const SOCKADDR*)&clientAddr, clientAddrLen, clientAddrBuffer, clientAddrBufferSize, nullptr, 0, NI_NUMERICHOST))//NI_NOFQDN  NI_NUMERICHOST
			{
				printserver("unable to resolve client address string");
			}

			printserver("connection accepted (%s)", clientAddrBuffer);
			
			//Handle incoming data on separate thread
			m_clients[id].clientThread = thread([this, id] {
				this->serverRecieve(id);
			});
			m_clients[id].clientThread.detach();
		}

		return true;
	}

	//Close all client connections
	bool serverDeinit()
	{
		//Close server listening socket
		if (m_listenSocket == INVALID_SOCKET)
		{
			printserver("listener socket already closed");
		}
		else
		{
			closesocket(m_listenSocket);
			m_listenSocket = INVALID_SOCKET;
			
			printserver("listener socket closed successfully");
		}

		for (int i = 0; i < MAX_CLIENT_CONTEXTS; i++)
		{
			//Close client accept socket
			if (m_clients[i].clientSocket == INVALID_SOCKET)
			{
				printserver("(%d) client socket already closed", i);
			}
			else
			{
				if (shutdown(m_clients[i].clientSocket, SD_BOTH) == SOCKET_ERROR)
				{
					printserver("shutdown failed: %d", WSAGetLastError());
					closesocket(m_clients[i].clientSocket);
					return false;
				}

				closesocket(m_clients[i].clientSocket);
				m_clients[i].clientSocket = INVALID_SOCKET;

				printserver("(%d) client socket closed successfully", i);
			}
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Send/recieve methods
	void serverRecieve(ClientId id)
	{
		int recieveResult = 0;
		// Receive data until the client connection is closed
		do
		{
			recieveResult = recv(m_clients[id].clientSocket, m_clients[id].recieveBuffer, RECV_BUFFER_SIZE, 0);

			if (recieveResult > 0)
			{
				m_server->onRecieve(id, m_clients[id].recieveBuffer, recieveResult);
			}
			else if (recieveResult < 0)
			{
				m_success = false;
			}

		} while ((recieveResult > 0));
	}

	bool serverSend(ClientId id, const void* data, size_t dataSize)
	{
		if (::send(m_clients[id].clientSocket, (const char*)data, (int)dataSize, 0) == SOCKET_ERROR)
		{
			if (m_clients[id].clientSocket == INVALID_SOCKET)
			{
				return false;
			}
			else
			{
				printserver("(id:%d) send failed: %d", id, WSAGetLastError());
				closesocket(m_clients[id].clientSocket);
				return true;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Impl(NetworkServer* server, const NetAddress::PortStr& port) :
		m_server(server)
	{
		platform::InitWinsock2();

		m_success = serverInit(port.c_str());

		//Wait until a client connection is accepted
		m_listenThread = thread([this]() {
			this->serverListen();
		});

		m_listenThread.detach();
	}

	~Impl()
	{
		serverDeinit();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkServer::NetworkServer(NetAddress::PortStr port) :
	pImpl(new Impl(this, port))
{}

NetworkServer::~NetworkServer()
{}

bool NetworkServer::sendAll(const void* data, size_t dataSize)
{
	for (int i = 0; i < MAX_CLIENT_CONTEXTS; i++)
	{
		pImpl->serverSend(i, data, dataSize);
	}

	return true;
}

bool NetworkServer::send(ClientId id, const void* data, size_t dataSize)
{
	return pImpl->serverSend(id, data, dataSize);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////