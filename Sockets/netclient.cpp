/*
	Client system
*/

#include "winsock.h"
#include "common.h"

#include "netclient.h"
#include "netserver.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Client implementation
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct NetworkClient::Impl
{
	NetworkClient* m_client = nullptr;

	SOCKET m_clientSocket = INVALID_SOCKET;
	bool m_success = false;
	
	thread m_recieveThread;
	char m_recieveBuffer[RECV_BUFFER_SIZE];

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Connect client to a server at a given address and port
	bool clientConnect(const char* serverAddress, const char* port)
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
			m_clientSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);

			if (m_clientSocket == INVALID_SOCKET)
			{
				printclient("failed to create socket. WSA error %d", WSAGetLastError());
			}

			//Get IP
			CHAR addressString[1024] = {};
			ZeroMemory(&addressString, sizeof(addressString));
			getAddressName(addressInfo, addressString, ARRAYSIZE(addressString));

			printclient("requesting connection to server (%s:%s)...", addressString, port);

			//Try to connect the socket using this address
			if (connectionStatus = connect(m_clientSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen))
			{
				printclient("connection failed (%d). Trying again...", connectionStatus);

				//If the next address in the linked list is null then there are no more connections to try
				if (addressInfo->ai_next == nullptr)
				{
					printclient("no more connections");
					return false;
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

		if (connectionStatus || m_clientSocket == INVALID_SOCKET)
		{
			printclient("failed to connect socket. WSA error %d", WSAGetLastError());
			closesocket(m_clientSocket);
			return false;
		}

		return true;
	}

	//Disconnect a client from the server
	bool clientDisconnect()
	{
		// shutdown the send half of the connection since no more data will be sent
		if (shutdown(m_clientSocket, SD_BOTH) == SOCKET_ERROR)
		{
			printclient("shutdown failed: %d", WSAGetLastError());
			closesocket(m_clientSocket);
			return false;
		}

		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;

		printclient("connection closed successfully");

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send/recieve data to/from a server
	void clientRecieve()
	{
		int recieveResult = 0;
		// Receive data until the server closes the connection
		do
		{
			recieveResult = recv(m_clientSocket, m_recieveBuffer, RECV_BUFFER_SIZE, 0);

			if (recieveResult > 0)
			{
				m_client->onRecieve(m_recieveBuffer, recieveResult);
			}
			else if (recieveResult < 0)
			{
				printclient("recieving failed: %d", WSAGetLastError());
			}

		} while ((recieveResult > 0));
	}

	void clientSend(const void* data, size_t dataSize)
	{
		if (::send(m_clientSocket, (const char*)data, (int)dataSize, 0) == SOCKET_ERROR)
		{
			printclient("send failed: %d", WSAGetLastError());
			closesocket(m_clientSocket);
			m_success = false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Impl(NetworkClient* client, const NetAddress& address) :
		m_client(client)
	{
		platform::InitWinsock2();

		m_success = clientConnect(
			address.getAddress().c_str(),
			address.getPort().c_str()
		);

		//todo: fix assertion failure
		//m_client->onConnect();

		if (!m_success)
			return;

		m_recieveThread = thread([this]() {
			this->clientRecieve();
		});
		m_recieveThread.detach();
	}

	~Impl()
	{
		//m_client->onDisconnect();
		m_success = clientDisconnect();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkClient::NetworkClient(const NetAddress& address) :
	pImpl(new Impl(this, address))
{}

NetworkClient::~NetworkClient() {}

void NetworkClient::send(const void* data, size_t dataSize)
{
	pImpl->clientSend(data, dataSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
