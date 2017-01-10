/*
	Server system
*/

#include "winsock.h"
#include "common.h"

#include "netclient.h"
#include "netserver.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Server implementation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct NetworkServer::Impl
{
	NetworkServer* m_server = nullptr;

	SOCKET m_listenSocket = INVALID_SOCKET;
	SOCKET m_acceptSocket = INVALID_SOCKET;
	bool m_success = false;
	bool m_recieving = true;

	thread m_recieveThread;
	char m_recieveBuffer[RECV_BUFFER_SIZE];

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
	bool serverAccept()
	{
		//Stores information about the client address
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);
		ZeroMemory(&clientAddr, clientAddrLen);

		//Wait for a client connection - creates a new client socket
		m_acceptSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &clientAddrLen);

		printserver("connections found.");

		if (m_acceptSocket == INVALID_SOCKET)
		{
			printserver("accept failed with error: %d", WSAGetLastError());
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

		//Close client accept socket
		if (m_acceptSocket == INVALID_SOCKET)
		{
			printserver("client socket already closed");
		}
		else
		{
			if (shutdown(m_acceptSocket, SD_BOTH) == SOCKET_ERROR)
			{
				printserver("shutdown failed: %d", WSAGetLastError());
				closesocket(m_acceptSocket);
				return false;
			}

			closesocket(m_acceptSocket);
			m_acceptSocket = INVALID_SOCKET;

			printserver("client socket closed successfully");
		}


		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Send/recieve methods
	void serverRecieve()
	{
		//Wait until a client connection is accepted
		serverAccept();

		int recieveResult = 0;
		// Receive data until the client connection is closed
		do
		{
			recieveResult = recv(m_acceptSocket, m_recieveBuffer, RECV_BUFFER_SIZE, 0);

			if (recieveResult > 0)
			{
				m_server->onRecieve(m_recieveBuffer, recieveResult);
			}
			else if (recieveResult < 0)
			{
				m_success = false;
			}

		} while ((recieveResult > 0));
	}

	void serverSend(const void* data, size_t dataSize)
	{
		if (::send(m_acceptSocket, (const char*)data, (int)dataSize, 0) == SOCKET_ERROR)
		{
			printclient("send failed: %d", WSAGetLastError());
			closesocket(m_acceptSocket);
			m_success = false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Impl(NetworkServer* server, const NetAddress::PortStr& port) :
		m_server(server)
	{
		platform::InitWinsock2();

		m_success = serverInit(port.c_str());

		m_recieveThread = thread([this]() {
			this->serverRecieve();
		});
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

void NetworkServer::send(const void* data, size_t dataSize)
{
	pImpl->serverSend(data, dataSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////