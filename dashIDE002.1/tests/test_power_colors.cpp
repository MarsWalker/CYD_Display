/**
 * @file test_power_colors.cpp
 *
 * Exercise actual JSON palette parsing with a deliberately tiny color resolver.
 * Tests isolate optional-property/default/alias behavior; the production color
 * resolver remains responsible for the complete supported color-name table.
 */

#include "../power_graph_json.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

static uint16_t resolve(const char* name){
    if(strcmp(name,"black")==0)return 0;
    if(strcmp(name,"red")==0)return 0xf800;
    if(strcmp(name,"blue")==0)return 0x001f;
    return uint16_t(strtoul(name,nullptr,0));
}
static PowerGraphColors parse(const char* json){
    JsonDocument doc;assert(!deserializeJson(doc,json));
    return ParsePowerGraphColors(doc.as<JsonObject>(),resolve);
}
int main(){
    auto c=parse("{}");assert(c.background==0x0861&&c.positive==0xffe0&&c.negative==0x07e0);
    c=parse(R"({"Background":"black","PositiveColor":"red","NegativeColor":"blue"})");
    assert(c.background==0&&c.positive==0xf800&&c.negative==0x001f);
    c=parse(R"({"PositiveColor":"red"})");assert(c.background==0x0861&&c.positive==0xf800&&c.negative==0x07e0);
    c=parse(R"({"Background":"","PositiveColor":null,"NegativeColor":123})");
    assert(c.background==0x0861&&c.positive==0xffe0&&c.negative==0x07e0);
    c=parse(R"({"background":"black"})");assert(c.background==0);
    c=parse(R"({"backgroud":"blue"})");assert(c.background==0x001f);
    c=parse(R"({"Background":"red","background":"blue","backgroud":"black"})");assert(c.background==0xf800);
    c=parse(R"({"Background":"0x0861","PositiveColor":"0xFFE0","NegativeColor":"0x07E0"})");
    assert(c.background==0x0861&&c.positive==0xffe0&&c.negative==0x07e0);
    puts("Graph color JSON tests passed.");
}
