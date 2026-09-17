/*
 * LCDDriver.hpp
 *
 * LTDC Framebuffer Driver (Double‑Buffer Rendering)
 * -------------------------------------------------
 * This class implements a lightweight framebuffer driver for the STM32F7
 * LTDC peripheral. It provides:
 *
 *   - double‑buffered rendering (activeFB + drawFB)
 *   - pixel‑level drawing
 *   - 8x12 bitmap font rendering
 *   - Bresenham line drawing
 *   - midpoint circle (outline + filled)
 *
 * Rendering Model:
 * ----------------
 * The driver maintains two framebuffers:
 *
 *      fb0 : currently displayed by LTDC (activeFB)
 *      fb1 : offscreen buffer used for drawing (drawFB)
 *
 * PixelEngine and LCDTask draw into drawFB. When a frame is complete,
 * activateFrameBuffer() swaps the buffers:
 *
 *      LTDC → drawFB
 *      drawFB ↔ activeFB
 *
 * This ensures:
 *   - flicker‑free rendering
 *   - deterministic frame updates
 *   - no tearing artifacts
 *
 * LTDC Integration:
 * -----------------
 * The LTDC layer framebuffer address is updated via:
 *
 *      __HAL_LTDC_LAYER(&hltdc, 0)->CFBAR = (uint32_t)drawFB;
 *      __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
 *
 * Memory Layout:
 * --------------
 * fb0 and fb1 are contiguous in memory:
 *
 *      fb0Ptr
 *      fb1Ptr = fb0Ptr + (width * height)
 *
 * Each pixel is a 32‑bit ARGB8888 value.
 *
 * Created on: Aug 12, 2026
 * Author: Stefan (310004)
 */


#pragma once
#include <stdint.h>
#include <cstring>
#include <cmath>
#include "Font8x12.hpp"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_ltdc.h"

extern LTDC_HandleTypeDef hltdc;

// ---------------------------------------------------------------------------
// ARGB8888 color enumeration
// ---------------------------------------------------------------------------
enum class Color : uint32_t {
    Black       = 0xFF000000,
    White       = 0xFFFFFFFF,
    Red         = 0xFFFF0000,
    Green       = 0xFF00FF00,
    Blue        = 0xFF0000FF,
    Yellow      = 0xFFFFFF00,
    Cyan        = 0xFF00FFFF,
    Magenta     = 0xFFFF00FF,
    Gray        = 0xFF808080,
    LightGray   = 0xFFD3D3D3,
    DarkGray    = 0xFF404040,
    Transparent = 0x00000000
};

// ---------------------------------------------------------------------------
// LCDDriver: double‑buffer framebuffer renderer
// ---------------------------------------------------------------------------
class LCDDriver {
public:
    // Singleton instance
    static LCDDriver& instance() {
        static LCDDriver pe;
        return pe;
    }

    // Initialize framebuffer pointers and dimensions
    inline void init(uint32_t* fb0Ptr, int width, int height) {
        fb0 = fb0Ptr;
        fb1 = fb0Ptr + (width * height);   // second buffer directly after fb0
        fb  = fb1;                         // start drawing into offscreen buffer
        activeFB = fb0;
        drawFB   = fb1;
        W = width;
        H = height;
        // TODO: verify memory size
        // TODO: clear buffers with memset
    }

    // Clear entire draw buffer
    inline void clear(Color color = Color::Black) {
        uint32_t c = static_cast<uint32_t>(color);
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
                drawFB[y * W + x] = c;
    }

    // Draw a single pixel
    inline void pixel(int x, int y, Color color) {
        if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H)
            drawFB[y * W + x] = static_cast<uint32_t>(color);
    }

    // Draw a single 8x12 character
    inline void char8x12(int x, int y, char c, Color color) {
        if (c < 32 || c > 126) return;
        const uint8_t* glyph = font8x12[c - 32];
        for (int row = 0; row < 12; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (1 << (7 - col)))
                    pixel(x + col, y + row, color);
            }
        }
    }

    // Draw a text string using 8x12 font
    inline void text8x12(int x, int y, const char* s, Color color) {
        int cx = x;
        for (size_t i = 0; i < strlen(s); i++) {
            char8x12(cx, y, s[i], color);
            cx += 9; // 8px glyph + 1px spacing
        }
    }

    // Swap active framebuffer (LTDC) with draw framebuffer
    inline void activateFrameBuffer() {
        // Switch LTDC to new buffer
        __HAL_LTDC_LAYER(&hltdc, 0)->CFBAR = (uint32_t)drawFB;
        __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);

        // Swap buffers
        uint32_t* tmp = activeFB;
        activeFB = drawFB;
        drawFB   = tmp;

        // PixelEngine now draws into the new offscreen buffer
        fb = drawFB;
    }

    // Bresenham line drawing
    inline void line(int x0, int y0, int x1, int y1, Color color) {
        int dx = fabs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -fabs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            pixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    // Midpoint circle (outline)
    inline void circle(int cx, int cy, int radius, Color color) {
        if (radius < 0) return;
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y) {
            pixel(cx + x, cy + y, color);
            pixel(cx + y, cy + x, color);
            pixel(cx - y, cy + x, color);
            pixel(cx - x, cy + y, color);
            pixel(cx - x, cy - y, color);
            pixel(cx - y, cy - x, color);
            pixel(cx + y, cy - x, color);
            pixel(cx + x, cy - y, color);

            y += 1;
            err += 1 + 2*y;
            if (2*(err - x) + 1 > 0) {
                x -= 1;
                err += 1 - 2*x;
            }
        }
    }

    // Filled circle using horizontal scanlines
    inline void fillCircle(int cx, int cy, int radius, Color color) {
        if (radius < 0) return;
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y) {
            for (int xi = cx - x; xi <= cx + x; ++xi) {
                pixel(xi, cy + y, color);
                pixel(xi, cy - y, color);
            }
            for (int xi = cx - y; xi <= cx + y; ++xi) {
                pixel(xi, cy + x, color);
                pixel(xi, cy - x, color);
            }

            y += 1;
            err += 1 + 2*y;
            if (2*(err - x) + 1 > 0) {
                x -= 1;
                err += 1 - 2*x;
            }
        }
    }

    // Default framebuffer start address (external SDRAM)
    uint32_t* pStartFrameBuffer = (uint32_t*)0xC0000000;

private:
    LCDDriver()
        : fb(nullptr), fb0(nullptr), fb1(nullptr),
          activeFB(nullptr), drawFB(nullptr),
          W(0), H(0) {}

    uint32_t* fb;        // current drawing buffer
    uint32_t* fb0;       // framebuffer 0 (LTDC)
    uint32_t* fb1;       // framebuffer 1 (offscreen)
    uint32_t* activeFB;  // currently displayed by LTDC
    uint32_t* drawFB;    // offscreen buffer for rendering

    int W, H;            // framebuffer dimensions
};
