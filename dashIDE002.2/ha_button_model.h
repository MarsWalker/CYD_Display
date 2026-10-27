/**
 * @file ha_button_model.h
 * Small platform-independent rules for HA buttons and contact edge detection.
 * Only explicitly supported on/off domains are accepted. Entity identifiers
 * are copied whole: truncating a control target could actuate another device.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

inline const char* HAButtonDomain(const char* entity) {
    if (!entity || strlen(entity) > 63) return nullptr;
    const char* domain = nullptr;
    size_t prefix = 0;
    if (strncmp(entity,"light.",6)==0) { domain="light"; prefix=6; }
    else if (strncmp(entity,"switch.",7)==0) { domain="switch"; prefix=7; }
    else if (strncmp(entity,"input_boolean.",14)==0) { domain="input_boolean"; prefix=14; }
    if (!domain || !entity[prefix]) return nullptr;
    for (const char* p=entity+prefix; *p; ++p)
        if (!((*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='_')) return nullptr;
    return domain;
}
inline bool HAButtonKnownState(const char* state) {
    return state && (strcmp(state,"on")==0 || strcmp(state,"off")==0);
}
inline bool HAButtonContains(int x,int y,int w,int h,int px,int py) {
    return x>=0 && y>=0 && w>=64 && h>=40 && px>=x && py>=y &&
        int64_t(px)<int64_t(x)+w && int64_t(py)<int64_t(y)+h;
}
struct TouchPressGate {
    bool held=false, emitted=false;
    uint32_t last=0;
    // A confirmed release is required after every accepted or suppressed press.
    // Transport failures must not call sample(false): they are not releases.
    bool sample(bool down,uint32_t now) {
        if (!down) { held=false; return false; }
        if (held) return false;
        held=true;
        if (emitted && uint32_t(now-last)<400) return false;
        emitted=true;last=now;return true;
    }
};
