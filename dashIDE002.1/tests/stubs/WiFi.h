/**
 * @file WiFi.h
 *
 * Native IPv4 parsing and deterministic DNS/Wi-Fi responses used by ping tests.
 * Addresses use the documentation-only TEST-NET range; no sockets are opened.
 */

#pragma once
#include "Arduino.h"
class IPAddress {
 uint8_t bytes[4]={};
public:
 bool fromString(const String& s) { unsigned a,b,c,d;char tail; if(sscanf(s.c_str(),"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)!=4 || a>255||b>255||c>255||d>255)return false;bytes[0]=a;bytes[1]=b;bytes[2]=c;bytes[3]=d;return true; }
 uint8_t operator[](int n)const{return bytes[n];}
 operator uint32_t()const{return bytes[0]|(bytes[1]<<8)|(bytes[2]<<16)|(uint32_t(bytes[3])<<24);}
};
#define WL_CONNECTED 3
struct WiFiMock {
 int connected=WL_CONNECTED;
 int status(){return connected;}
 int hostByName(const char* host,IPAddress& addr){if(std::string(host)=="dns-ok"){addr.fromString("192.0.2.11");return 1;}return 0;}
};
inline WiFiMock WiFi;
