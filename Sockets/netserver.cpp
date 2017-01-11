/*
	Server system
*/

#include "winsock.h"
#include "common.h"

#include "netclient.h"
#include "netserver.h"

#include <functional>
#include <memory>
#include <algorithm>
#include <list>
#include <mutex>
#include <cassert>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	ClientContext class - class that represents a connected client
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct IClientListener
{
	virtual void onRecieve(ClientId id, const void* data, size_t dataSize) = 0;
	virtual void onDisconnect(ClientId id) = 0;
	virtual void onConnect(ClientId id) = 0;
};

class ClientContext
{
private:
	
	SOCKET m_socket = INVALID_SOCKET;
	thread m_recieveThread;
	char m_recieveBuffer[RECV_BUFFER_SIZE] = {0};
	mutex m_recieveMutex;

	IClientListener& m_listener;

	void handleRecieve()
	{
		int recieveResult = 0;

		m_listener.onConnect(getId());

		// Receives data until the client connection is closed
		do
		{
			recieveResult = recv(m_socket, m_recieveBuffer, RECV_BUFFER_SIZE, 0);

			if (recieveResult > 0)
			{
				m_listener.onRecieve(getId(), m_recieveBuffer, recieveResult);
			}
			else if (recieveResult < 0)
			{
				printserver("(%p) error recieve result %d", getId(), recieveResult);
			}

		} while ((recieveResult > 0));

		m_listener.onDisconnect(getId());
	}

public:
	
	ClientContext(IClientListener& listener, SOCKET sock) :
		m_socket(sock),
		m_listener(listener)
	{
		m_recieveThread = thread(&ClientContext::handleRecieve, this);
		m_recieveThread.detach();
	}

	~ClientContext()
	{
		//lock_guard<mutex>lk(m_recieveMutex);

		const ClientId id = getId();

		if (shutdown(m_socket, SD_BOTH) == SOCKET_ERROR)
		{
			printserver("(%p) shutdown failed: %d", id, WSAGetLastError());
		}

		closesocket(m_socket);
		printserver("(%p) client socket closed successfully", id);

		m_socket = INVALID_SOCKET;
	}
	
	ClientId getId() const
	{
		return (ClientId)m_socket;
	}

	SOCKET getSocket() const
	{
		return m_socket;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NetworkServer private implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NetworkServer::Impl : public IClientListener
{
	NetworkServer* m_server = nullptr;

	SOCKET m_listenSocket = INVALID_SOCKET;
	thread m_listenThread;

	list<unique_ptr<ClientContext>> m_clients;
	recursive_mutex m_clientsMutex;

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
			
			printserver("connections found.");

			if (s == INVALID_SOCKET)
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

			lock_guard<recursive_mutex>lk(m_clientsMutex);
			m_clients.push_back(unique_ptr<ClientContext>(new ClientContext(*this, s)));
		}

		return true;
	}

	//Close all client connections
	bool serverDeinit()
	{
		//Disconnect all clients
		{
			lock_guard<recursive_mutex>lk(m_clientsMutex);
			m_clients.clear();
		}

		//Close server listening socket
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
			
		printserver("listener socket closed successfully");

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//Find a client context with a given id
	ClientContext* getClient(ClientId id)
	{
		lock_guard<recursive_mutex>lk(m_clientsMutex);

		typedef unique_ptr<ClientContext> c_t;
		auto it = find_if(m_clients.begin(), m_clients.end(), [=](const c_t& c) { return c->getId() == id; });
		
		if (it == m_clients.end())
		{
			return nullptr;
		}
		else
		{
			return it->get();
		}
	}

	//Closes client socket and removes it from client list
	void destroyClient(ClientId id)
	{
		lock_guard<recursive_mutex>lk(m_clientsMutex);

		typedef unique_ptr<ClientContext> c_t;
		auto it = find_if(m_clients.begin(), m_clients.end(), [=](const c_t& c) { return c->getId() == id; });
		
		if (it != m_clients.end())
		{
			it->reset();
			m_clients.erase(it);
		}
	}

	//Gets the Winsock socket handle of a client with a given id
	SOCKET getClientSocket(ClientId id)
	{
		if (ClientContext* con = getClient(id))
		{
			return con->getSocket();
		}

		return INVALID_SOCKET;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Send/recieve methods

	bool serverSend(ClientId id, const void* data, size_t dataSize)
	{
		SOCKET sock = getClientSocket(id);

		if (::send(sock, (const char*)data, (int)dataSize, 0) == SOCKET_ERROR)
		{
			printserver("(%p) send failed: %d", id, WSAGetLastError());
			return false;
		}
		
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Event handlers
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void onRecieve(ClientId id, const void* data, size_t dataSize) override
	{
		m_server->onRecieve(id, data, dataSize); //Pass event up to NetworkServer::onRecieve()
	}

	void onDisconnect(ClientId id) override
	{
		m_server->onDisconnect(id);
		destroyClient(id); //remove client context from list when they disconnect
	}

	void onConnect(ClientId id) override
	{
		m_server->onConnect(id);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ctor/dtor
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
// NetworkServer interface
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkServer::NetworkServer(NetAddress::PortStr port) :
	pImpl(new Impl(this, port))
{}

NetworkServer::~NetworkServer()
{}

bool NetworkServer::sendAll(const void* data, size_t dataSize)
{
	lock_guard<recursive_mutex>lk(pImpl->m_clientsMutex);

	for (auto& c : pImpl->m_clients)
	{
		pImpl->serverSend(c->getId(), data, dataSize);
	}

	return true;
}

bool NetworkServer::send(ClientId id, const void* data, size_t dataSize)
{
	return pImpl->serverSend(id, data, dataSize);
}


int NetworkServer::getClientCount() const
{
	lock_guard<recursive_mutex>lk(pImpl->m_clientsMutex);
	return (int)pImpl->m_clients.size();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
