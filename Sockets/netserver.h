/*
	TCP server class
*/

#pragma once

#include <memory>
#include "netcommon.h"

class NetworkServer
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;

public:

	NetworkServer(const SNetAddress& address);
	~NetworkServer();
};
