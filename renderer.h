// OpenGL 2.x fixed-function draw: starfield (Rubik-style), tetractys mesh, optional turn animation.

#pragma once

#include <SFML/OpenGL.hpp>

#include "tetrahedron_mesh.h"

#include <array>

/** UX / Aesthetic: in-flight twist sweep (vertex 120° or edge 180°) for visible motion before state commits. */
struct TurnAnimDraw {
    bool active = false;
    bool isVertex = true;
    int index = 0;
    /** Tetractys band 0..kVertexBands-1 for vertex turns; ignored for edges. */
    int layer = 0;
    int vertexDir = 1;
    float currentDeg = 0.f;
};

/** UI / Aesthetic: OpenGL scene (lighting, materials, starfield); UX: optional animated twist deformation. */
class Renderer {
public:
    Renderer();

    /** Aesthetic: GL state (lights, clear color, depth). */
    void initialize();
    /** UI: viewport when the window resizes. */
    void resize(int width, int height);

    /** UX: mouse-wheel zoom (distance clamped). */
    void handleMouseWheel(int delta);

    /** UI: projection and view; Aesthetic: black clear + starfield, then mesh-ready matrices. */
    void beginScene(float yawDeg, float pitchDeg, float rollDeg);

    /** Aesthetic: per-sticker materials and colors; UX: anim mask deforms only the turning band. */
    void drawMesh(const TetMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                    const TurnAnimDraw* anim);

private:
    /** Aesthetic: background star points before mesh (called from beginScene). */
    void drawStars();

    /** UX: camera distance along -Z after perspective (wheel zoom). */
    float cameraDistance_;
    static constexpr float kFovYDeg = 45.f;
    static constexpr float kCameraDistMin = 2.2f;
    static constexpr float kCameraDistMax = 14.f;
};
