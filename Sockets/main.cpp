/*
	Socket communication program
*/

#include <iostream>

#include "netclient.h"
#include "netserver.h"

#include "stringhelpers.h"
#include "cmdargs.h"

#include <Windows.h>
#include <consoleapi.h>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Client/Server message classes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MessageServer : public NetworkServer
{
public:

	MessageServer(const NetAddress::PortStr& port) :
		NetworkServer(port)
	{}

	//When a message is recieved from a client, echo it back to all connected clients
	void onRecieve(ClientId id, const void* data, size_t dataSize) override
	{
		cout.write((const char*)data, dataSize);
		cout << "\n";
		this->sendAll(data, dataSize);
	}

	void run()
	{
		string buf;
		do
		{
			getline(cin, buf);
		} while (buf != "#exit");
	}
};

class MessageClient : public NetworkClient
{
private:

	string m_username;

public:

	MessageClient(const NetAddress& addr) :
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
	
	//Print message when it is recieved from server
	void onRecieve(const void* data, size_t dataSize) override
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
		cout.write((const char*)data, dataSize);
		cout << "\n";
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}

	void run()
	{
		string buf;
		while (true)
		{
			getline(cin, buf);

			if (!buf.empty())
			{
				if (buf == "#exit")
				{
					break;
				}

				buf = "[" + m_username + "]: " + buf;
				this->send(buf.c_str(), buf.size()); //dispatch to server
			}
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
		MessageServer server(addr.getPort());
		server.run();
	}
	else if (isClient && !isServer)
	{
		MessageClient client(addr);
		client.run();
	}
	else
	{
		printf("Invalid arguments, program must specify either --server or --client\n");
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
