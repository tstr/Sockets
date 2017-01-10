/*
	Winsock initialization
*/

#include "winsock.h"

#include <exception>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct WinSock2
{
	WinSock2()
	{
		WSADATA wsadata;
		if (WSAStartup(MAKEWORD(2, 2), &wsadata))
		{
			char buf[128] = { 0 };
			sprintf(buf, "failed to initialize winsock. error code: %d\n", WSAGetLastError());
			printf(buf);
			throw std::exception(buf);
		}

		printf(wsadata.szDescription);
		printf("\n");
	}

	~WinSock2()
	{
		WSACleanup();
	}
};


namespace platform
{
	bool InitWinsock2()
	{
		static WinSock2 init;
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Obtains address name as a string
void getAddressName(addrinfo* info, char* buffer, size_t szbuffer, int flag)
{
	if (getnameinfo(info->ai_addr, sizeof(SOCKADDR), buffer, (DWORD)szbuffer, nullptr, 0, flag))
	{
		printf("unable to resolve address name\n");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
