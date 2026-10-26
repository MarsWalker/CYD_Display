/**
 * @file test_ping.cpp
 *
 * Exercise the real ping source/worker code with deterministic platform stubs.
 * A fake scheduler ends one worker round by throwing, permitting inspection
 * without creating live tasks. Read failures and a reload during a request
 * are simulated; the test must never send real ICMP or mDNS traffic.
 */

// Native behavioural tests: real ping_sources.h + ArduinoJson, mocked hardware.
#include <cassert>
#define LOG_H
#define logE(...) ((void)0)
#define logW(...) ((void)0)
#define logIf(...) ((void)0)
#include "../ping_sources.h"
static void configure(const char* body) {JsonDocument doc;assert(!deserializeJson(doc,body));ParsePingSources(doc);}
static void cycle(){try{PingWorker(nullptr);}catch(int n){assert(n==42);}}
static void reloadDuringPing(){configure(R"({"DataSources":[{"Type":"Ping","Id":"replacement","Host":"192.0.2.12"}]})");}
int main(){
 configure(R"({"DataSources":[{"Type":"Ping","Id":"one","Host":"192.0.2.10","Refresh":10000}]})");
 assert(PingValue("one")=="--");assert(PingValue("missing")=="--");
 cycle();assert(PingValue("one")=="OK");assert(created==1&&deleted==1);
 assert(lastConfig.count==2&&lastConfig.timeout_ms==2000);
 cycle();assert(created==1); // refresh throttles repeated requests
 testNow+=10000;replies=0;cycle();assert(PingValue("one")=="FAIL");
 testNow+=10000;replies=1;cycle();assert(PingValue("one")=="OK");
 WiFi.connected=0;assert(PingValue("one")=="FAIL");testNow+=10000;cycle();assert(PingValue("one")=="FAIL");
 WiFi.connected=WL_CONNECTED;
 configure(R"({"DataSources":[{"Type":"Ping","Id":"one","Host":"example-device"}]})");
 cycle();assert(PingValue("one")=="OK");assert(MDNS.queries.back()=="example-device");
 assert(PingHost("example-device.local"));assert(PingHost("dns-ok"));assert(!PingHost("absent.local"));
 configure(R"({"DataSources":[{"Type":"Ping","Id":"one","Host":"192.0.2.10"}]})");
 duringPing=reloadDuringPing;cycle();assert(PingValue("one")=="--");assert(PingValue("replacement")=="--");
 cycle();assert(PingValue("replacement")=="OK");
 configure(R"({"DataSources":[{"Type":"Ping","Id":"a","Host":"x","Refresh":0},{"Type":"Ping","Id":"a","Host":"y"},{"Type":"Ping","Id":"","Host":"z"}]})");
 assert(g_pingCount==1&&g_pingSources[0].refresh==1000);
 configure(R"({"DataSources":[{"Type":"Ping","Id":"a","Host":"192.0.2.10"}]})");failCreate=true;cycle();assert(PingValue("a")=="FAIL");failCreate=false;
 configure("{}");assert(g_pingCount==0&&PingValue("a")=="--");
 assert(created==deleted);
 puts("Ping tests passed: replies, timeouts, refresh, Wi-Fi, DNS/mDNS, reload, validation, allocation failure.");
}
