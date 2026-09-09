#pragma once

#include "Core/Core.h"

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>

namespace Muk {

/** Build a purple/gold Muk logo HICON at runtime (no external .ico required). */
inline HICON CreateMukAppIcon(int size = 32) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!color || !bits) return nullptr;

    auto* px = static_cast<unsigned char*>(bits);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            int i = (y * size + x) * 4;
            bool edge = x <= 1 || y <= 1 || x >= size - 2 || y >= size - 2;
            bool barL = x >= 2 && x <= 5 && y >= 4 && y <= size - 5;
            bool barR = x >= size - 6 && x <= size - 3 && y >= 4 && y <= size - 5;
            bool top = y >= 4 && y <= 10 && x >= 2 && x <= size - 3;
            bool mid = (x >= size / 2 - 2 && x <= size / 2 + 1) && y >= 10 && y <= size - 8;
            bool m = barL || barR || top || mid;
            if (edge) {
                px[i + 0] = 100; px[i + 1] = 30; px[i + 2] = 60; px[i + 3] = 255;
            } else if (m) {
                px[i + 0] = 40; px[i + 1] = 180; px[i + 2] = 220; px[i + 3] = 255;
            } else {
                px[i + 0] = 180; px[i + 1] = 50; px[i + 2] = 140; px[i + 3] = 255;
            }
        }
    }

    // 1-bit mask (all opaque)
    int maskStride = ((size + 31) / 32) * 4;
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = mask;
    ii.hbmColor = color;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}

} // namespace Muk
#endif
