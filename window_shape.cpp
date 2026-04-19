// UI: Windows SetWindowRgn — upside-down triangle matching the drawable (client) area.

#include "window_shape.h"

#include <SFML/Graphics.hpp>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32)

/**
 * UI: top-left of the client area in window-relative pixels (non-client chrome offset via ClientToScreen).
 * Returns false if Win32 rect queries fail.
 */
bool computeClientOriginInWindow(HWND hwnd, LONG& outOriginX, LONG& outOriginY, LONG& outClientW, LONG& outClientH) {
    RECT clientR{};
    if (GetClientRect(hwnd, &clientR) == 0) {
        return false;
    }
    POINT topLeft{};
    topLeft.x = clientR.left;
    topLeft.y = clientR.top;
    static_cast<void>(ClientToScreen(hwnd, &topLeft));
    RECT winR{};
    if (GetWindowRect(hwnd, &winR) == 0) {
        return false;
    }
    outOriginX = topLeft.x - winR.left;
    outOriginY = topLeft.y - winR.top;
    outClientW = clientR.right - clientR.left;
    outClientH = clientR.bottom - clientR.top;
    return true;
}

/** UI: flat top, tip at bottom center — matches GL viewport silhouette for the puzzle window. */
void fillUpsideDownTrianglePoints(LONG ox, LONG oy, LONG cw, LONG ch, POINT outPts[3]) {
    outPts[0].x = ox;
    outPts[0].y = oy;
    outPts[1].x = ox + cw - 1;
    outPts[1].y = oy;
    outPts[2].x = ox + cw / 2;
    outPts[2].y = oy + ch - 1;
}

#endif

/** Maps the client rectangle to an upside-down triangle in window space and applies it as the HWND region. */
void applyUpsideDownTriangleWindowShape(sf::RenderWindow& window) {
#if defined(_WIN32)
    const sf::Vector2u sz = window.getSize();
    if (sz.x < 4 || sz.y < 4) {
        return;
    }
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    if (hwnd == nullptr) {
        return;
    }

    LONG ox = 0;
    LONG oy = 0;
    LONG cw = 0;
    LONG ch = 0;
    if (!computeClientOriginInWindow(hwnd, ox, oy, cw, ch)) {
        return;
    }

    POINT pts[3]{};
    fillUpsideDownTrianglePoints(ox, oy, cw, ch, pts);

    HRGN rgn = CreatePolygonRgn(pts, 3, WINDING);
    if (rgn != nullptr) {
        static_cast<void>(SetWindowRgn(hwnd, rgn, TRUE));
    }
#else
    (void)window;
#endif
}
