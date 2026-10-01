#include <ArduinoJson.h>

int CurrentScreen          = 0; // Pointer state tracking active runtime display page
int RequestedScreen        = 1; // Pointer state tracking active runtime display page
int totalScreens           = 5; // Evaluated total dynamic pages available in configuration

uint16_t screen_backgound_color = TFT_BLACK;
DynamicJsonDocument ScreenDoc(16384);

bool g_HAConnected = false;           // Logic handshake flag checking connection with Home Assistant
JsonArray screens;
extern bool FirstDraw = true;
extern uint32_t ScreenRefresh = 5000;
extern uint32_t ScreenLastRefresh = 0;

// ============================================================================
// GLOBAL DATA STORAGE: HOME ASSISTANT ENTITY CACHE
// ============================================================================
struct EntityCache {
    String entity_id;  // The Home Assistant entity code (e.g., "sensor.living_room_temp")
    String lastState;  // Stores the last known state string to detect real changes
};

extern const int MAX_GLOBAL_ENTITIES = 20;               // Maximum number of HA sensors we can track simultaneously
//EntityCache globalCache[MAX_GLOBAL_ENTITIES];    // Allocation of the static entity array cache
int globalCacheCount = 0;                        // Counter for currently active cached entities


