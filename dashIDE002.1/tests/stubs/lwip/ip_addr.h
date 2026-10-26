/**
 * @file lwip/ip_addr.h
 *
 * Small native address representation for testing IPv4 target construction.
 * This is not a replacement lwIP implementation for production firmware.
 */

#pragma once
#include <cstdint>
using ip_addr_t=uint32_t;
#define IP_ADDR4(dst,a,b,c,d) (*(dst)=uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16)|(uint32_t(d)<<24))
