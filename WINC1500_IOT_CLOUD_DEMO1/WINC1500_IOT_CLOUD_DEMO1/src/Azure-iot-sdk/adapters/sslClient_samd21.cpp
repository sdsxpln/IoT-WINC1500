// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <IPAddress.h>
#include "sslClient_samd21.h"
#include "azure_c_shared_utility/xlogging.h"

#include "driver/include/m2m_wifi.h"
static WiFiSSLClient sslClient;

void sslClient_setTimeout(unsigned long timeout)
{
    sslClient.setTimeout(timeout);
}

uint8_t sslClient_connected(void)
{
    return (uint8_t)sslClient.connected();
}

int sslClient_connect(uint32_t ipAddress, uint16_t port)
{
    IPAddress ip = IPAddress(ipAddress);
    return (int)sslClient.connect(ip, port);
}

void sslClient_stop(void)
{
    sslClient.stop();
}

size_t sslClient_write(const uint8_t *buf, size_t size)
{
    return sslClient.write(buf, size);
}

size_t sslClient_print(const char* str)
{
    return sslClient.print(str);
}

int sslClient_read(uint8_t *buf, size_t size)
{
    return sslClient.read(buf, size);
}

int sslClient_available(void)
{
    return sslClient.available();
}

uint8_t sslClient_hostByName(const char* hostName, uint32_t* ipAddress)
{
    IPAddress ip;
    uint8_t result = WiFi.hostByName(hostName, ip);
    (*ipAddress) = (uint32_t)ip;
    return result;
}

