// Bronze-style materials; starfield + black clear aligned with Rubik reference app.
// Comments tag UI (viewport, draw order), UX (zoom, animation masking), Aesthetic (light, color, materials).

#include "renderer.h"

#include "math3.h"
#include "turn_geometry.h"

#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

/** Aesthetic: diffuse RGBA for palette index 0..3 (four tetrahedron face colors in solved state). */
void diffuseForFace(int face, float out[4]) {
    // Aesthetic: four distinguishable face colors (cool red, bronze, gold, coral) for solved-state read.
    const float pal[4][3] = {
        {0.82f, 0.10f, 0.22f},
        {0.66f, 0.42f, 0.22f},
        {0.90f, 0.74f, 0.28f},
        {0.92f, 0.30f, 0.18f},
    };
    const int f = face % 4;
    out[0] = pal[f][0];
    out[1] = pal[f][1];
    out[2] = pal[f][2];
    out[3] = 1.0f;
}

/** UX: degrees to radians for partial-turn deformation angles. */
float degToRad(float d) {
    return d * static_cast<float>(M_PI / 180.0);
}

/** UI: depth test, black clear, smooth shading, and low scene-wide ambient (contrast on bronze materials). */
void setupFramebufferAndShadeModel() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Aesthetic: black clear + low global ambient for sculptural contrast on the mesh.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_SMOOTH);

    const float globalAmb[4] = {0.025f, 0.025f, 0.028f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

/** Aesthetic: single directional GL_LIGHT0 — warm key from upper-left; strong diffuse/spec for metal read. */
void setupKeyLight() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    const float lightPos[4] = {-0.85f, 0.9f, 0.95f, 0.0f};
    const float lightAmb[4] = {0.06f, 0.06f, 0.065f, 1.0f};
    const float lightDiff[4] = {1.75f, 1.65f, 1.55f, 1.0f};
    const float lightSpec[4] = {2.1f, 2.0f, 1.95f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
}

} // namespace

Renderer::Renderer() : cameraDistance_(3.5f) {}

void Renderer::initialize() {
    setupFramebufferAndShadeModel();
    setupKeyLight();
}

// UX: zoom in/out with clamped distance so the mesh stays framed.
void Renderer::handleMouseWheel(int delta) {
    cameraDistance_ += static_cast<float>(delta) * 0.2f;
    cameraDistance_ = std::max(kCameraDistMin, std::min(kCameraDistMax, cameraDistance_));
}

// UI: GL viewport tracks window size.
void Renderer::resize(int width, int height) {
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, width, height);
}

namespace {

/** Aesthetic: emit `count` points on a sphere shell (fixed RNG seed set by caller). */
void emitStarPointsOnSphere(int count, float pointSize, float cr, float cg, float cb) {
    glPointSize(pointSize);
    glColor3f(cr, cg, cb);
    for (int i = 0; i < count; ++i) {
        const float theta = static_cast<float>(std::rand() % 628) / 100.0f;
        const float phi = static_cast<float>(std::rand() % 314) / 100.0f;
        constexpr float radius = 50.0f;
        const float x = radius * std::sin(phi) * std::cos(theta);
        const float y = radius * std::sin(phi) * std::sin(theta);
        const float z = radius * std::cos(phi);
        glVertex3f(x, y, z);
    }
}

} // namespace

/** Aesthetic: star shell at large radius — matches Rubik `drawStars` (fixed seed). */
void Renderer::drawStars() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glBegin(GL_POINTS);
    std::srand(42);
    emitStarPointsOnSphere(150, 2.0f, 1.0f, 1.0f, 1.0f);
    emitStarPointsOnSphere(15, 3.0f, 1.0f, 1.0f, 0.9f);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// UI: projection + view; Aesthetic: draw starfield behind the mesh.
void Renderer::beginScene(float yawDeg, float pitchDeg, float rollDeg) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const double aspect =
        static_cast<double>(viewport[2]) / static_cast<double>(viewport[3] > 0 ? viewport[3] : 1);
    gluPerspective(static_cast<double>(kFovYDeg), aspect, 0.08, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -cameraDistance_);
    glRotatef(pitchDeg, 1.0f, 0.0f, 0.0f);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glRotatef(rollDeg, 0.0f, 0.0f, 1.0f);

    drawStars();
}

namespace {

/**
 * UX: which micro-triangles belong to the in-flight twist; fills mask[sticker]=1 for moving stickers.
 * moving is scratch from vertexLayerTurnStickerIndices / edgeTurnStickerIndices.
 */
void fillAnimStickerMask(const TetMesh& mesh, const TurnAnimDraw* anim, std::vector<int>& moving,
                         std::vector<char>& mask) {
    mask.assign(static_cast<std::size_t>(kStickerCount), 0);
    const bool useAnim = anim != nullptr && anim->active;
    if (!useAnim) {
        return;
    }
    if (anim->isVertex) {
        const int L = anim->layer >= 0 && anim->layer < kVertexBands ? anim->layer : 0;
        vertexLayerTurnStickerIndices(mesh, anim->index, L, moving);
    } else {
        edgeTurnStickerIndices(mesh, anim->index, moving);
    }
    for (int id : moving) {
        mask[static_cast<std::size_t>(id)] = 1;
    }
}

} // namespace

void Renderer::drawMesh(const TetMesh& mesh, const std::array<int, kStickerCount>& colors, float scale,
                        const TurnAnimDraw* anim) {
    thread_local std::vector<int> moving;
    thread_local std::vector<char> mask;
    fillAnimStickerMask(mesh, anim, moving, mask);
    const bool useAnim = anim != nullptr && anim->active;

    // Aesthetic: low material ambient + bright spec/shininess for metallic stickers.
    const float ambMat[4] = {0.04f, 0.035f, 0.03f, 1.0f};
    const float specMat[4] = {0.92f, 0.88f, 0.82f, 1.0f};

    const float angRad =
        useAnim ? degToRad(anim->currentDeg) : 0.f;

    glPushMatrix();
    glScalef(scale, scale, scale);

    // UX: only stickers in the moving band deform; Aesthetic: normals/positions follow the rigid twist.
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < kStickerCount; ++i) {
        const int pal = colors[static_cast<std::size_t>(i)];
        float diff[4];
        diffuseForFace(pal, diff);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambMat);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specMat);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 92.0f);

        const MeshTriangle& tri = mesh.triangles[static_cast<std::size_t>(i)];
        Vec3 n = tri.normal;
        if (useAnim && mask[static_cast<std::size_t>(i)]) {
            if (anim->isVertex) {
                n = vertexTurnNormal(mesh, n, anim->index, anim->vertexDir, angRad);
            } else {
                n = edgeTurnNormal(mesh, n, anim->index, angRad);
            }
        }
        glNormal3f(n.x, n.y, n.z);

        for (int k = 0; k < 3; ++k) {
            Vec3 p = tri.pos[static_cast<std::size_t>(k)];
            if (useAnim && mask[static_cast<std::size_t>(i)]) {
                if (anim->isVertex) {
                    p = vertexTurnPosition(mesh, p, anim->index, anim->vertexDir, angRad);
                } else {
                    p = edgeTurnPosition(mesh, p, anim->index, angRad);
                }
            }
            glVertex3f(p.x, p.y, p.z);
        }
    }
    glEnd();
    glPopMatrix();
}
