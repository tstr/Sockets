/*
	TCP client class
*/

#pragma once

#include <memory>
#include "netaddr.h"

typedef const char* Address;

class NetworkClient
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;
	
protected:
	
	//Event handlers
	virtual void onRecieve(const void* data, size_t dataSize) = 0;	//Called when data is recieved from server
	virtual void onConnect() = 0;									//Called when client connects
	virtual void onDisconnect() = 0;								//Called when client disconnects

public:

	NetworkClient() = delete;
	NetworkClient(const NetAddress& address);
	~NetworkClient();

	void send(const void* data, size_t dataSize);
};
