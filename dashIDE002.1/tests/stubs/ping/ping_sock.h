/**
 * @file ping/ping_sock.h
 *
 * Deterministic finite-session ping API. Start invokes the completion callback
 * immediately, with configurable reply counts, allocation failure and a hook
 * that simulates a concurrent configuration reload. Session counters verify
 * resource cleanup without opening sockets.
 */

#pragma once
#include "lwip/ip_addr.h"
using esp_ping_handle_t=void*;
struct esp_ping_config_t {unsigned count,interval_ms,timeout_ms,task_stack_size;ip_addr_t target_addr;};
#define ESP_PING_DEFAULT_CONFIG() {5,1000,1000,2048,0}
#define ESP_OK 0
#define ESP_PING_PROF_REPLY 1
struct esp_ping_callbacks_t {void* cb_args;void(*on_ping_end)(esp_ping_handle_t,void*);};
inline esp_ping_callbacks_t cb;
inline esp_ping_config_t lastConfig;
inline unsigned replies=2,created=0,deleted=0;
inline bool failCreate=false;
inline void(*duringPing)()=nullptr;
inline int esp_ping_new_session(const esp_ping_config_t* c,const esp_ping_callbacks_t* b,esp_ping_handle_t* h){if(failCreate)return -1;lastConfig=*c;cb=*b;*h=(void*)1;++created;return 0;}
inline int esp_ping_start(esp_ping_handle_t h){if(duringPing){auto f=duringPing;duringPing=nullptr;f();}cb.on_ping_end(h,cb.cb_args);return 0;}
inline void esp_ping_delete_session(esp_ping_handle_t){++deleted;}
inline void esp_ping_get_profile(esp_ping_handle_t,int,void* out,unsigned){*(uint32_t*)out=replies;}
