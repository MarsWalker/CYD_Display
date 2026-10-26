/**
 * @file black_panel.h
 *
 * C ABI used by the C++ display facade to reach the black-panel transport.
 * The application initializes this transport once. BlackPanelWriteRows accepts
 * native panel rows and does not return until the DMA buffer can be reused.
 * Touch coordinates returned here have not yet been rotated into UI space.
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void BlackPanelInit(void);
void BlackPanelWriteRows(int y, int rows, const uint16_t *pixels);
void BlackPanelInvert(bool invert);
bool BlackTouchInit(void);
bool BlackTouchRead(uint16_t *x, uint16_t *y);
#ifdef __cplusplus
}
#endif
