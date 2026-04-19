// UI: optional platform window outline (Windows: clip HWND to an upside-down triangle).

#pragma once

namespace sf {
class RenderWindow;
}

/** UI: clip the OS window to an upside-down triangle (flat top, tip at bottom). No-op on non-Windows. */
void applyUpsideDownTriangleWindowShape(sf::RenderWindow& window);
