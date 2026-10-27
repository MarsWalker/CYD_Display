/**
 * @file ha_buttons.h
 * One bounded control request, consumed by the existing data task under the
 * engine mutex. No HTTP is sent from hit-testing or rendering. Requests are
 * copied by entity ID, never by an ObjectInfo pointer that a reload invalidates.
 * A toggle is sent exactly once, with no automatic retry after an ambiguous
 * timeout. Only authoritative HA GET responses update the displayed state.
 */
#pragma once
#include "ha_button_model.h"
#ifndef HA_BUTTON_NATIVE_TEST
#include "objects.h"
#include "datacache.h"
#endif

struct HAButtonRequest {
    char entity[64] = {};
    bool pending=false, error=false;
    uint32_t completed=0;
};
static HAButtonRequest g_haButtonRequest;

inline void ResetHAButtonRequest() { g_haButtonRequest=HAButtonRequest{}; }
inline bool HAButtonPending(const char* entity) {
    return g_haButtonRequest.pending && strcmp(entity,g_haButtonRequest.entity)==0;
}
inline bool HAButtonError(const char* entity) {
    return g_haButtonRequest.error && strcmp(entity,g_haButtonRequest.entity)==0;
}
// Caller holds EngineLock. A button consumes its rectangle even when disabled,
// unknown, offline or busy, so it never accidentally triggers screen navigation.
inline bool HandleHAButtonTouch(int x,int y) {
    for (int i=g_objectCount-1;i>=0;--i) {
        ObjectInfo& obj=g_objects[i];
        if (obj.type!=OBJ_DRAWHABUTTON || obj.x+obj.x2>tft.width() || obj.y+obj.y2>tft.height() ||
            !HAButtonContains(obj.x,obj.y,obj.x2,obj.y2,x,y)) continue;
        if (!obj.zoom || !wifiConnected() || !HAButtonKnownState(EntityStateGet(obj.entities)) ||
            g_haButtonRequest.pending) return true;
        // Also bound distinct rapid taps after a service completes.
        if (g_haButtonRequest.completed && uint32_t(millis()-g_haButtonRequest.completed)<600) return true;
        strlcpy(g_haButtonRequest.entity,obj.entities,sizeof(g_haButtonRequest.entity));
        g_haButtonRequest.pending=true;
        g_haButtonRequest.error=false;
        return true;
    }
    return false;
}
inline bool ReadHAButtonState(const char* entity) {
    String base(HA_URL);if (!base.endsWith("/")) base+="/";
    int code=-1;
    String body=httpGetBearer(base+"api/states/"+entity,HA_TOKEN,&code,3000,true);
    JsonDocument doc;
    if (code!=200 || deserializeJson(doc,body)) return false;
    const char* state=doc["state"] | "";
    if (!state[0]) return false;
    EntityStateSet(entity,state);
    return HAButtonKnownState(state);
}
inline void ProcessHAButtonRequest() {
    if (!g_haButtonRequest.pending) return;
    const char* entity=g_haButtonRequest.entity;
    const char* domain=HAButtonDomain(entity);
    int code=-1;
    if (domain && wifiConnected()) {
        String base(HA_URL);if (!base.endsWith("/")) base+="/";
        JsonDocument payload;payload["entity_id"]=entity;
        String body;serializeJson(payload,body);
        httpPostBearer(base+"api/services/"+domain+"/toggle",body,HA_TOKEN,&code,3000);
    }
    // Read back even after a timeout: the command may have reached HA.
    // This is a read, never a retry of the state-changing POST.
    bool read=wifiConnected() && ReadHAButtonState(entity);
    g_haButtonRequest.error=!(code>=200 && code<300 && read);
    g_haButtonRequest.pending=false;
    g_haButtonRequest.completed=millis();
    logIf("[button] %s toggle HTTP=%d read=%d",entity,code,read);
    // Force the configured HA group to poll again for delayed device updates.
    for (int i=0;i<g_dataSourceCount;++i)
        if (strcmp(g_dataSources[i].type,"HA")==0) g_dataCache[i].lastFetchMs=0;
}
