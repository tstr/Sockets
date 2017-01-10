/*
	Server/Client common data
*/

#pragma once

#include "winsock.h"

#include <thread>
#include <mutex>

#define SEND_BUFFER_SIZE 8192
#define RECV_BUFFER_SIZE 8192

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging helpers
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename ... args_t>
inline void printclient(const char* format, args_t ... args)
{
	//Convert timestamp into hours/minutes/seconds
	time_t stamp = time(0);
	tm t;
	localtime_s(&t, &stamp);
	char timestr[200];
	strftime(timestr, sizeof(timestr), "[%H:%M:%S]", &t);

	printf(((std::string)timestr + " CLIENT: " + format + "\n").c_str(), args...);
}

template<typename ... args_t>
inline void printserver(const char* format, args_t ... args)
{
	//Convert timestamp into hours/minutes/seconds
	time_t stamp = time(0);
	tm t;
	localtime_s(&t, &stamp);
	char timestr[200];
	strftime(timestr, sizeof(timestr), "[%H:%M:%S]", &t);

	printf(((std::string)timestr + " SERVER: " + format + "\n").c_str(), args...);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
