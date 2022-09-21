# NTPServer
An Arduino-targetted NTP Server class that supports both basic NTP requests, and a limited subset of control requests. I built this library to also serve up timezone information via NTP management requests.

Note that the documentation is currently a work in progress.

## Quick Start

You'll need 3 things to get this up and running:

1. A network such as `WiFi` or `Ethernet` (with a UDP handler)
2. An external time source
3. An instance of NTPServer

Future me will put in an example of starting up the wifi and udp handler.

To initialize the NTP Server, you need to know what stratum and time reference you are using. In my case, I was obtaining my time source from a GPS receiver. This puts me in stratum 1 as a GPS reference:

```
NTPServer myServer("GPS", L_NTP_STRAT_PRIMARY);   // Note: L_NTP_STRAT_PRIMARY=1
WiFiUDP ntpUdpHandler;
```

Now we can initialize the library with the `begin` method:

```
ntpUdpHandler.begin(123);          // 123 is NTP port
myServer.begin(ntpUdpHandler);
```

The next step is to add a call to `update` in your main loop:

```
void loop() {
	myServer.update();
}

```

Lastly, the server will need to know the current time, which brings us to:

## Setting Server Time

#### void setReferenceTime(struct tm refTime, t_ntpSysClock refTimeMillis)

This method sets the reference time from an external source. The two parameters involved are as follows:

1. `refTime`: The reference time (i.e. the time as parsed from the GPS receiver)
2. `refTimeMillis`: The processor time (i.e. a snapshot of `millis()`) that the reference time was taken at.

The more often this is set, the more accurate the server will be. In the case of a GPS time server, you should capture `refTimeMillis` at the rising edge of the PPS signal, then call `setReferenceTime` once the serial time data has been decoded.

#### void setReferenceTime(struct tm refTime)

Sets the reference time, taking the current value of millis() for convenience (note: this method is not as accurate).

## NTP Configuration

#### setStratum(char stratum)

Sets the current stratum of the server. Valid values are 1 through 16. The stratum is reported back to clients with every request.

#### setMaxPollInterval(int pollIntervalSeconds)

Sets the maximum poll interval that will be reported to clients. Note that the actual interval that will be locked in will be to the closest power of 2, as the NTP protocol definition of poll interval is 2 to the power of the interval.

#### setServerPrecision(double precisionInSeconds)

Sets the reported server precision, in seconds.

#### setRootDelay(double delayInSeconds)

Sets the root delay of the time source.

#### setRootDispersion(double dispersionInSeconds)

Sets the root dispersion of the time source.

#### setReferenceId(const char * referenceId)

Sets the NTP reference ID, in character format. Note that this function will fail if the reference ID is more than 4 bytes (exclusing null terminator).

## Reading Variables

To enable variable reading through NTP control packets, you need to hook into the `onReadVariable` callback. Until the callback is hooked into, any control requests against the server will fail. To set the callback, do the following:

```
int myCallbackFunction(const char *var, char *lpBuffer, int cbBuffer)
{
	int result = L_NTP_R_ERROR;
	
	if (!strncasecmp(var, "MY_VARIABLE_NAME_GOES_HERE", strlen(var)))
	{
		// Note: Take care not to exceed the buffer space as defined by cbBuffer
		strcpy(lpBuffer, "DATA GOES HERE");
		
		result = L_NTP_R_SUCCESS; // Inform server class that we have a valid response
	}
	
	return result;
}

myServer.onReadVariable(myCallbackFunction);

```
