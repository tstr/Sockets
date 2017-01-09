/*
	TCP client class
*/

#pragma once

#include <memory>
#include "netcommon.h"

typedef const char* Address;

class NetworkClient
{
private:

	struct Impl;
	std::unique_ptr<Impl> pImpl;

public:

	NetworkClient(const SNetAddress& address);
	~NetworkClient();
};
