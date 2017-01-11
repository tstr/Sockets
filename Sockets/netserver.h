/*
	TCP server class
*/

#pragma once

#include <memory>
#include "netaddr.h"

typedef void* ClientId;

class NetworkServer
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;

protected:

	//Event handlers
	virtual void onRecieve(ClientId id, const void* data, size_t dataSize) = 0;
	virtual void onDisconnect(ClientId id) = 0;
	virtual void onConnect(ClientId id) = 0;

public:

	NetworkServer() = delete;
	NetworkServer(NetAddress::PortStr port);
	~NetworkServer();

	bool send(ClientId id, const void* data, size_t dataSize);
	bool sendAll(const void* data, size_t dataSize);

	int getClientCount() const;
};
