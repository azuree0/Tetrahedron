// SFML 3 + OpenGL: tetractys bands 1–6, Rubik-style stars, animated turns.
// Comments tag UI (layout/overlay), UX (controls, motion, feedback), Aesthetic (via Renderer).
// Structure: anonymous-namespace keyboard routing, then main() wiring (events → animation → renderFrame).

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <optional>

#include "pyraminx_state.h"
#include "renderer.h"
#include "tetrahedron_mesh.h"
#include "window_shape.h"

namespace {

/** Mesh corner index for Q–Y shortcuts (mesh.corners[3]; screen position depends on orbit). */
constexpr int kBottomRightVertex = 3;

/** mesh.corners[1]: A-H run vertex layers 0..5 (tetractys tip..base) at this corner. */
constexpr int kVertexOne = 1;

/** mesh.corners[2]: plain Z-N = vertex layers 0..5; Ctrl+Z..N = edges 0..5. */
constexpr int kVertexTwo = 2;

/** Tetractys band 6 (0-based layer 5): Num6 / Numpad6, Y, H, N — base strip + opposite face (see turn_geometry). */
constexpr std::uint8_t kOuterVertexBand = static_cast<std::uint8_t>(kVertexBands - 1);

// UI: window pixel size for the GL viewport and SFML surface (larger canvas for puzzle + HUD).
constexpr unsigned kWindowW = 1920;
constexpr unsigned kWindowH = 1080;
// UX: turn animation angular speed (how fast a twist completes on screen).
constexpr float kAnimDegPerSec = 300.f;

/**
 * UX: enqueue a vertex-layer twist: one tetractys band (0 = tip … kOuterVertexBand = base) at a mesh corner.
 */
template <typename StartTurnFn>
void startVertexLayerMove(StartTurnFn&& startTurn, int meshCorner, std::uint8_t band, int vertexTwistDir) {
    startTurn({PuzzleMove::Kind::VertexLayer, static_cast<std::uint8_t>(meshCorner), band,
               static_cast<std::int8_t>(vertexTwistDir)});
}

/** UX: enqueue a 180° edge flip on edge index 0..5 (pairing of tetrahedron corners). */
template <typename StartTurnFn>
void startEdgeMove(StartTurnFn&& startTurn, int edgeIndex) {
    startTurn({PuzzleMove::Kind::Edge, static_cast<std::uint8_t>(edgeIndex), 0, 1});
}

/**
 * UX: Q–Y twist bands 1–6 at mesh.corners[3] (fixed “bottom-right” apex in the default view).
 * Returns true if the key was one of Q..Y.
 */
template <typename StartTurnFn>
bool tryHandleKeysMeshCorner3Qwerty(sf::Keyboard::Key key, int vertexTwistDir, StartTurnFn&& startTurn) {
    using K = sf::Keyboard::Key;
    if (key == K::Q) {
        startVertexLayerMove(startTurn, kBottomRightVertex, 0, vertexTwistDir);
    } else if (key == K::W) {
        startVertexLayerMove(startTurn, kBottomRightVertex, 1, vertexTwistDir);
    } else if (key == K::E) {
        startVertexLayerMove(startTurn, kBottomRightVertex, 2, vertexTwistDir);
    } else if (key == K::R) {
        startVertexLayerMove(startTurn, kBottomRightVertex, 3, vertexTwistDir);
    } else if (key == K::T) {
        startVertexLayerMove(startTurn, kBottomRightVertex, 4, vertexTwistDir);
    } else if (key == K::Y) {
        startVertexLayerMove(startTurn, kBottomRightVertex, kOuterVertexBand, vertexTwistDir);
    } else {
        return false;
    }
    return true;
}

/** UX: U/I/O/P select which mesh corner 0..3 the number-row / numpad bands apply to. Returns true if handled. */
bool tryHandleUiopVertexSelect(sf::Keyboard::Key key, int& activeVertexOut) {
    using K = sf::Keyboard::Key;
    if (key == K::U) {
        activeVertexOut = 0;
    } else if (key == K::I) {
        activeVertexOut = 1;
    } else if (key == K::O) {
        activeVertexOut = 2;
    } else if (key == K::P) {
        activeVertexOut = 3;
    } else {
        return false;
    }
    return true;
}

/**
 * UX: main-row 1–6 and numpad 1–6 apply tetractys bands at the UIOP-selected corner (activeVertex).
 * Returns true if the key was a number-row or numpad digit 1..6.
 */
template <typename StartTurnFn>
bool tryHandleUiopNumberRowAndNumpad(sf::Keyboard::Key key, int vertexTwistDir, int activeVertex,
                                     StartTurnFn&& startTurn) {
    using K = sf::Keyboard::Key;
    if (key == K::Num1 || key == K::Numpad1) {
        startVertexLayerMove(startTurn, activeVertex, 0, vertexTwistDir);
    } else if (key == K::Num2 || key == K::Numpad2) {
        startVertexLayerMove(startTurn, activeVertex, 1, vertexTwistDir);
    } else if (key == K::Num3 || key == K::Numpad3) {
        startVertexLayerMove(startTurn, activeVertex, 2, vertexTwistDir);
    } else if (key == K::Num4 || key == K::Numpad4) {
        startVertexLayerMove(startTurn, activeVertex, 3, vertexTwistDir);
    } else if (key == K::Num5 || key == K::Numpad5) {
        startVertexLayerMove(startTurn, activeVertex, 4, vertexTwistDir);
    } else if (key == K::Num6 || key == K::Numpad6) {
        startVertexLayerMove(startTurn, activeVertex, kOuterVertexBand, vertexTwistDir);
    } else {
        return false;
    }
    return true;
}

/**
 * UX: A–H twist bands 1–6 at mesh.corners[1].
 * Returns true if the key was one of A..H.
 */
template <typename StartTurnFn>
bool tryHandleKeysMeshCorner1Ah(sf::Keyboard::Key key, int vertexTwistDir, StartTurnFn&& startTurn) {
    using K = sf::Keyboard::Key;
    if (key == K::A) {
        startVertexLayerMove(startTurn, kVertexOne, 0, vertexTwistDir);
    } else if (key == K::S) {
        startVertexLayerMove(startTurn, kVertexOne, 1, vertexTwistDir);
    } else if (key == K::D) {
        startVertexLayerMove(startTurn, kVertexOne, 2, vertexTwistDir);
    } else if (key == K::F) {
        startVertexLayerMove(startTurn, kVertexOne, 3, vertexTwistDir);
    } else if (key == K::G) {
        startVertexLayerMove(startTurn, kVertexOne, 4, vertexTwistDir);
    } else if (key == K::H) {
        startVertexLayerMove(startTurn, kVertexOne, kOuterVertexBand, vertexTwistDir);
    } else {
        return false;
    }
    return true;
}

/**
 * UX: Z–N without Ctrl = vertex layers at mesh.corners[2]; with Ctrl = edge moves 0..5 (same letter order).
 * Returns true if the key was Z..N.
 */
template <typename StartTurnFn>
bool tryHandleKeysMeshCorner2OrEdges(sf::Keyboard::Key key, bool ctrlHeld, int vertexTwistDir,
                                     StartTurnFn&& startTurn) {
    using K = sf::Keyboard::Key;
    if (key == K::Z) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 0);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, 0, vertexTwistDir);
        }
    } else if (key == K::X) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 1);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, 1, vertexTwistDir);
        }
    } else if (key == K::C) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 2);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, 2, vertexTwistDir);
        }
    } else if (key == K::V) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 3);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, 3, vertexTwistDir);
        }
    } else if (key == K::B) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 4);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, 4, vertexTwistDir);
        }
    } else if (key == K::N) {
        if (ctrlHeld) {
            startEdgeMove(startTurn, 5);
        } else {
            startVertexLayerMove(startTurn, kVertexTwo, kOuterVertexBand, vertexTwistDir);
        }
    } else {
        return false;
    }
    return true;
}

/**
 * UX: Space scrambles, Backspace undoes, Tab solves to uniform faces, Enter resets puzzle and cancels in-flight anim.
 */
void handleGlobalPuzzleCommands(sf::Keyboard::Key key, PyraminxState& state, TurnAnimDraw& anim,
                                sf::Clock& scrambleClock) {
    using K = sf::Keyboard::Key;
    if (key == K::Space) {
        if (!anim.active) {
            state.scramble(40, static_cast<unsigned>(scrambleClock.getElapsedTime().asMilliseconds()));
        }
    } else if (key == K::Backspace) {
        if (!anim.active) {
            state.undo();
        }
    } else if (key == K::Tab) {
        if (!anim.active) {
            state.autoComplete();
        }
    } else if (key == K::Enter) {
        anim.active = false;
        anim.currentDeg = 0.f;
        state.reset();
    }
}

/**
 * UX: full puzzle keyboard map — per-row apex twists, UIOP target, ZN/edges, world commands.
 * Shift inverts vertex twist direction; modifiers are read by the caller (vdir, ctrl).
 */
template <typename StartTurnFn>
void handlePuzzleKeyboardInput(sf::Keyboard::Key key, bool ctrlHeld, int vertexTwistDir, int& activeVertex,
                               PyraminxState& state, TurnAnimDraw& anim, sf::Clock& scrambleClock,
                               StartTurnFn&& startTurn) {
    if (tryHandleKeysMeshCorner3Qwerty(key, vertexTwistDir, startTurn)) {
        return;
    }
    if (tryHandleUiopVertexSelect(key, activeVertex)) {
        return;
    }
    if (tryHandleUiopNumberRowAndNumpad(key, vertexTwistDir, activeVertex, startTurn)) {
        return;
    }
    if (tryHandleKeysMeshCorner1Ah(key, vertexTwistDir, startTurn)) {
        return;
    }
    if (tryHandleKeysMeshCorner2OrEdges(key, ctrlHeld, vertexTwistDir, startTurn)) {
        return;
    }
    handleGlobalPuzzleCommands(key, state, anim, scrambleClock);
}

/** UI: request depth buffer, stencil, and OpenGL 2.1 for fixed-function rendering. */
sf::ContextSettings makeGlContextSettings() {
    sf::ContextSettings s;
    s.depthBits = 24;
    s.stencilBits = 8;
    s.majorVersion = 2;
    s.minorVersion = 1;
    return s;
}

/**
 * UI: load common Windows fonts and construct optional HUD text (size/color; position set each frame in drawHudOverlay).
 * Returns false if no font file could be opened (HUD disabled).
 */
bool tryLoadHudFont(sf::Font& fontOut, std::optional<sf::Text>& hudOut) {
    const bool ok = fontOut.openFromFile(R"(C:\Windows\Fonts\arial.ttf)") ||
                    fontOut.openFromFile(R"(C:\Windows\Fonts\calibri.ttf)");
    if (!ok) {
        return false;
    }
    constexpr unsigned kHudFontPx = 24u;
    hudOut.emplace(fontOut, "", kHudFontPx);
    hudOut->setFillColor(sf::Color(235, 235, 240));
    return true;
}

/** UX: copy PuzzleMove fields into TurnAnimDraw for Renderer::drawMesh (vertex band, edge index, direction). */
void syncAnimFieldsFromMove(TurnAnimDraw& anim, const PuzzleMove& m) {
    if (m.kind == PuzzleMove::Kind::VertexLayer) {
        anim.isVertex = true;
        anim.index = m.index;
        anim.layer = m.layer < kVertexBands ? static_cast<int>(m.layer) : 0;
        anim.vertexDir = m.dir >= 0 ? 1 : -1;
    } else {
        anim.isVertex = false;
        anim.index = m.index;
        anim.layer = 0;
        anim.vertexDir = 1;
    }
}

/** UX: begin a twist animation when idle; stores the move in pending for apply() at animation end. */
void beginTurnIfIdle(TurnAnimDraw& anim, PuzzleMove& pending, const PuzzleMove& m) {
    if (anim.active) {
        return;
    }
    pending = m;
    anim.active = true;
    anim.currentDeg = 0.f;
    syncAnimFieldsFromMove(anim, m);
}

/** UX: advance in-flight twist; commits sticker permutation when vertex (120°) or edge (180°) cap is reached. */
void advanceTurnAnimation(float dt, TurnAnimDraw& anim, PyraminxState& state, PuzzleMove& pending) {
    if (!anim.active) {
        return;
    }
    anim.currentDeg += kAnimDegPerSec * dt;
    const float cap = anim.isVertex ? 120.f : 180.f;
    if (anim.currentDeg >= cap) {
        anim.currentDeg = cap;
        state.apply(pending);
        anim.active = false;
        anim.currentDeg = 0.f;
    }
}

/** UX: apply mouse delta to orbit angles; clamps pitch so the camera does not flip past the poles. */
void applyOrbitDrag(float& yawDeg, float& pitchDeg, int dx, int dy) {
    constexpr float kOrbitSensitivity = 0.35f;
    yawDeg += static_cast<float>(dx) * kOrbitSensitivity;
    pitchDeg += static_cast<float>(dy) * kOrbitSensitivity;
    if (pitchDeg > 89.f) {
        pitchDeg = 89.f;
    }
    if (pitchDeg < -89.f) {
        pitchDeg = -89.f;
    }
}

/** UI: draw the single-line ASCII help string centered along the top edge (push/pop GL states for SFML). */
void drawHudOverlay(sf::RenderWindow& window, sf::Text& hud) {
    hud.setString(
        "1-6: vertex 1 | q-y: vertex 2 | a-h: vertex 3 | z-n: vertex 4 | Shift+: reverse | Space: "
        "scramble | Tab: complete");
    // UI: horizontal center of client area; origin at top-center of glyph bounds so the line sits below a small top pad.
    const sf::FloatRect lb = hud.getLocalBounds();
    hud.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y});
    constexpr float kHudTopPad = 14.f;
    hud.setPosition({static_cast<float>(window.getSize().x) * 0.5f, kHudTopPad});
    window.pushGLStates();
    window.draw(hud);
    window.popGLStates();
}

/**
 * UX: drain one frame of SFML events — window chrome, resize (re-clips upside-down triangle), mouse orbit, zoom, keys.
 * startTurn is invoked for puzzle keys (after Escape routing).
 */
template <typename StartTurnFn>
void processFrameEvents(sf::RenderWindow& window, Renderer& renderer, PyraminxState& state, TurnAnimDraw& anim,
                        int& activeVertex, float& yawDeg, float& pitchDeg, bool& drag, sf::Vector2i& dragPos,
                        sf::Clock& scrambleClock, StartTurnFn&& startTurn) {
    while (const std::optional<sf::Event> ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* rs = ev->getIf<sf::Event::Resized>()) {
            renderer.resize(static_cast<int>(rs->size.x), static_cast<int>(rs->size.y));
            applyUpsideDownTriangleWindowShape(window);
        }
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (mb->button == sf::Mouse::Button::Left) {
                drag = true;
                dragPos = mb->position;
            }
        }
        if (const auto* mr = ev->getIf<sf::Event::MouseButtonReleased>()) {
            if (mr->button == sf::Mouse::Button::Left) {
                drag = false;
            }
        }
        if (const auto* mm = ev->getIf<sf::Event::MouseMoved>()) {
            if (drag) {
                const int dx = mm->position.x - dragPos.x;
                const int dy = mm->position.y - dragPos.y;
                dragPos = mm->position;
                applyOrbitDrag(yawDeg, pitchDeg, dx, dy);
            }
        }
        if (const auto* wheel = ev->getIf<sf::Event::MouseWheelScrolled>()) {
            renderer.handleMouseWheel(static_cast<int>(wheel->delta));
        }
        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            const auto key = kp->code;
            if (key == sf::Keyboard::Key::Escape) {
                window.close();
            } else {
                const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
                const bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
                const int vdir = shift ? -1 : 1;
                handlePuzzleKeyboardInput(key, ctrl, vdir, activeVertex, state, anim, scrambleClock, startTurn);
            }
        }
    }
}

/**
 * UI: full frame — sync viewport, clear + stars + mesh, optional HUD, swap buffers.
 */
void renderFrame(sf::RenderWindow& window, Renderer& renderer, float yawDeg, float pitchDeg, float rollDeg,
                 const TetMesh& mesh, PyraminxState& state, TurnAnimDraw& anim, std::optional<sf::Text>& hud) {
    renderer.resize(static_cast<int>(window.getSize().x), static_cast<int>(window.getSize().y));
    static_cast<void>(window.setActive(true));
    renderer.beginScene(yawDeg, pitchDeg, rollDeg);
    renderer.drawMesh(mesh, state.colors(), 1.0f, anim.active ? &anim : nullptr);
    if (hud.has_value()) {
        drawHudOverlay(window, *hud);
    }
    window.display();
}

} // namespace

int main() {
    const sf::ContextSettings settings = makeGlContextSettings();

    // UI: borderless — clip to an upside-down triangle (flat top, tip at bottom); Escape closes (no title bar close).
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(kWindowW, kWindowH)), "Tetrahedron", sf::Style::None,
                            sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    static_cast<void>(window.setActive(true));
    applyUpsideDownTriangleWindowShape(window);

    Renderer renderer;
    renderer.initialize();

    // UI: world scale for the mesh (with Renderer camera) sets on-screen size of the puzzle.
    const TetMesh mesh = buildTetrahedronMesh(1.6f);
    PyraminxState state(mesh);

    TurnAnimDraw anim{};
    PuzzleMove pending{};

    int activeVertex = 0;

    // UX: initial orbit — face 0 (cool red) centered, tetractys tip (corner 0) up; roll aligns framing to triangle window.
    float yawDeg = -135.f;
    float pitchDeg = 35.f;
    const float rollDeg = -60.f;
    bool drag = false;
    sf::Vector2i dragPos{};

    sf::Clock frameClock;
    sf::Clock scrambleClock;
    sf::Font font;
    std::optional<sf::Text> hud;
    static_cast<void>(tryLoadHudFont(font, hud));

    auto startTurn = [&](const PuzzleMove& m) { beginTurnIfIdle(anim, pending, m); };

    while (window.isOpen()) {
        const float dt = frameClock.restart().asSeconds();

        processFrameEvents(window, renderer, state, anim, activeVertex, yawDeg, pitchDeg, drag, dragPos, scrambleClock,
                           startTurn);

        advanceTurnAnimation(dt, anim, state, pending);

        renderFrame(window, renderer, yawDeg, pitchDeg, rollDeg, mesh, state, anim, hud);
    }

    return 0;
}