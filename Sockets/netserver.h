/*
	TCP server class
*/

#pragma once

#include <memory>
#include "netaddr.h"

class NetworkServer
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;

protected:

	//Events
	virtual void onRecieve(const void* data, size_t dataSize) = 0;

public:

	NetworkServer() = delete;
	NetworkServer(NetAddress::PortStr port);
	~NetworkServer();

	virtual void send(const void* data, size_t dataSize);
};
