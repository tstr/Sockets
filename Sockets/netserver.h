/*
	TCP server class
*/

#pragma once

#include <memory>
#include "netaddr.h"

typedef size_t ClientId;

class NetworkServer
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;

protected:

	//Events
	virtual void onRecieve(ClientId id, const void* data, size_t dataSize) = 0;

public:

	NetworkServer() = delete;
	NetworkServer(NetAddress::PortStr port);
	~NetworkServer();

	bool send(ClientId id, const void* data, size_t dataSize);
	bool sendAll(const void* data, size_t dataSize);
};
