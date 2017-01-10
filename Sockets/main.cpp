/*
	Socket communication program
*/

#include <iostream>

#include "netclient.h"
#include "netserver.h"

#include "stringhelpers.h"
#include "cmdargs.h"

#include <Windows.h>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Client/Server classes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Server : public NetworkServer
{
private:

	string m_username;

public:

	Server(const NetAddress::PortStr& port) :
		NetworkServer(port)
	{
		DWORD bufsz = 256;
		char buf[256] = { 0 };
		GetUserNameA(buf, &bufsz);
		m_username = buf;
	}

	void onRecieve(const void* data, size_t dataSize) override
	{
		cout.write((const char*)data, dataSize);
		cout << "\n";
	}

	void run()
	{
		string buf;
		while (true)
		{
			getline(cin, buf);
			
			if (!buf.empty())
			{
				buf = "[" + m_username + "]: " + buf;
				this->send(buf.c_str(), buf.size());
			}
		}
	}
};

class Client : public NetworkClient
{
private:

	string m_username;

public:

	Client(const NetAddress& addr) :
		NetworkClient(addr)
	{
		DWORD bufsz = 256;
		char buf[256] = { 0 };
		GetUserNameA(buf, &bufsz);
		m_username = buf;
	}

	void onClose() override
	{

	}

	void onRecieve(const void* data, size_t dataSize) override
	{
		cout.write((const char*)data, dataSize);
		cout << "\n";
	}

	void run()
	{
		string buf;
		while (true)
		{
			getline(cin, buf);
			buf = "[" + m_username + "]: " + buf;
			this->send(buf.c_str(), buf.size());
		}
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	CommandLineArgs args(argv, argc);

	string address;
	string port("27015");

	if (args.isArgumentTag("address"))
		address = args.getArgumentValue("address");
	if (args.isArgumentTag("port"))
		port = args.getArgumentValue("port");
	
	bool isServer = args.isArgumentTag("server");
	bool isClient = args.isArgumentTag("client");

	printf("Address: %s\n", address.c_str());
	printf("Port: %s\n", port.c_str());

	NetAddress addr(port, address);

	if (isServer && !isClient)
	{
		Server server(addr.getPort());
		server.run();
	}
	else if (isClient && !isServer)
	{
		Client client(addr);
		client.run();
	}
	else
	{
		printf("Invalid arguments, program must specify either --server or --client\n");
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
