/**
 * @file Arduino.h
 *
 * Minimal native-test Arduino surface: String-like storage, monotonic fake
 * time, bounds helpers and a deterministic device identifier. This header is
 * selected only through tests/run.sh include paths; it is not a firmware shim.
 */

#pragma once
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <vector>
class String : public std::string {
public:
 using std::string::string;
 String(const std::string& s):std::string(s){}
 String(uint32_t v, int) : std::string(std::to_string(v)) {}
 bool endsWith(const char* s) const { return size() >= strlen(s) && compare(size()-strlen(s),strlen(s),s)==0; }
 void remove(size_t n) { erase(n); }
};
#define HEX 16
#define constrain(x,a,b) std::min(std::max((x),(a)),(b))
inline size_t strlcpy(char* dst,const char* src,size_t n) { size_t len=strlen(src); if(n){size_t c=std::min(n-1,len);memcpy(dst,src,c);dst[c]=0;}return len; }
inline uint32_t testNow=100;
inline uint32_t millis(){return testNow;}
struct EspMock {uint64_t getEfuseMac(){return 1;}};
inline EspMock ESP;
