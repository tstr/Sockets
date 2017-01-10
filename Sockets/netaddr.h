/*
	Network address helper
*/

#pragma once

#include <string>

class NetAddress
{
private:

	//65553
	enum { PortLength = 6 };
	enum { AddrLength = 46 };

	char m_port[PortLength];
	char m_addr[AddrLength];

public:

	typedef std::string PortStr;
	typedef std::string AddrStr;

	NetAddress()
	{
		//Zeroe out struct
		memset(this, 0, sizeof(NetAddress));
	}

	NetAddress(PortStr port, AddrStr address) :
		NetAddress::NetAddress()
	{
		strcpy_s(m_port, port.c_str());
	
		if (address.size() > 0)
			strcpy_s(m_addr, address.c_str());
	}

	PortStr getPort() const { return m_port; }
	AddrStr getAddress() const { return (m_addr[0] == 0) ? nullptr : m_addr; }

	void getPort(PortStr& port) const
	{
		port = m_port;
	}
	
	void getAddress(AddrStr& addr) const
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
