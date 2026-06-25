#ifndef TOUCH_H
#define TOUCH_H

// ============================================================================
// TOUCH DRIVER CONDITIONAL COMPILATION
// This architecture acts as a unified abstraction layer. Depending on the 
// macro defined in your project settings (e.g., build_flags in PlatformIO), 
// it includes the specific driver implementation while keeping the main 
// application code agnostic of the hardware details.
// ============================================================================

#if defined(TOUCH_DRIVER_XPT2046)
    // Include driver for the standard XPT2046 resistive touch controller
    #include "touch_xpt2046.h"

#elif defined(TOUCH_DRIVER_XPT2046_2_4)
    // Include driver optimized for the XPT2046 controller on 2.4-inch displays
    #include "touch_xpt2046_2_4.h"

#elif defined(TOUCH_DRIVER_XPT2046_3_2)
    // Include driver optimized for the XPT2046 controller on 3.2-inch displays
    #include "touch_xpt2046_3_2.h"

#elif defined(TOUCH_DRIVER_FT6236)
    // Include driver for the FT6236 capacitive touch controller (commonly FocalTech)
    #include "touch_ft6236.h"

#elif defined(TOUCH_DRIVER_CST820)
    // Include driver for the CST820 capacitive touch controller
    #include "touch_cst820.h"

#elif defined(TOUCH_DRIVER_GT911)
    // Include driver for the Goodix GT911 capacitive touch controller
    // Note: Standardized to lowercase to match the project's naming convention
    #include "touch_gt911.h"

#else
    // ============================================================================
    // COMPILATION GUARD
    // If no valid touch driver macro is defined, halt compilation and trigger 
    // a clear error message to guide the developer.
    // ============================================================================
    #error "Touch driver undefined! Please define one of the valid TOUCH_DRIVER_* macros in your configuration."
#endif

#endif // TOUCH_H