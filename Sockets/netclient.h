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
	
	virtual void onRecieve(const void* data, size_t dataSize) = 0;	//Called when data is recieved from server
	virtual void onClose() = 0;										//Called when client closes connection

public:

	NetworkClient() = delete;
	NetworkClient(const NetAddress& address);
	~NetworkClient();

	virtual void send(const void* data, size_t dataSize);
};
