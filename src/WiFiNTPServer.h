#pragma once

/*
 * WiFiNTPServer.h
 *
 * Implements a basic WiFi-compatible NTP Server. Tested on ESP32.
 */

#include "NTPServer.h"
#include <WiFiUdp.h>

class WiFiNTPServer : public NTPServer
{
	public:
	
	WiFiNTPServer();
	WiFiNTPServer(const char *referenceId, const char stratum) : NTPServer(referenceId, stratum)
	{
		
	}
	
	void begin(int portNum)
	{
		_udp = new WiFiUDP();
		_udp->begin(portNum);
	}
	
	void begin()
	{
		begin(123);
	}
};
