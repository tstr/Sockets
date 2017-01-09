/*
	Network address helper
*/

#pragma once

#include <string>

class SNetAddress
{
private:

	//65553
	enum { PortLength = 6 };
	enum { AddrLength = 46 };

	char m_port[PortLength];
	char m_addr[AddrLength];

public:

	typedef const char* PortStr;
	typedef const char* AddrStr;

	SNetAddress()
	{
		//Zeroe out struct
		memset(this, 0, sizeof(SNetAddress));
	}

	SNetAddress(PortStr port, AddrStr address = nullptr) :
		SNetAddress::SNetAddress()
	{
		strcpy_s(m_port, port);
	
		if (address)
			strcpy_s(m_addr, address);
	}

	PortStr getPort() const { return m_port; }
	AddrStr getAddress() const { return (m_port[0] == 0) ? nullptr : m_port; }

	void getPort(std::string& port) const
	{
		port = m_port;
	}
	
	void getAddress(std::string& addr) const
	{
		if (m_port[0] == 0)
		{
			addr = "";
		}
		else
		{
			addr = m_port;
		}
	}
};
