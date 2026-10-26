/**
 * @file ESPmDNS.h
 *
 * Record mDNS queries and return deterministic test answers without a network.
 * The query list lets tests verify .local fallback and suffix handling.
 */

#pragma once
#include "WiFi.h"
struct MdnsMock {
 std::vector<std::string> queries;
 bool begin(const char*){return true;}
 IPAddress queryHost(const char* host,unsigned){queries.push_back(host);IPAddress addr;if(std::string(host)=="example-device")addr.fromString("192.0.2.13");return addr;}
};
inline MdnsMock MDNS;
