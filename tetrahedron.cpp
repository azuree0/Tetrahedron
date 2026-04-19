// Tetrahedron puzzle implementation: geometry, caps, and 120-degree vertex twists.

#include "tetrahedron.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

float dot3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

sf::Vector3f cross3(const sf::Vector3f& a, const sf::Vector3f& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float len3(const sf::Vector3f& v) { return std::sqrt(dot3(v, v)); }

sf::Vector3f norm3(const sf::Vector3f& v) {
    const float L = len3(v);
    if (L <= 1e-8f) {
        return {0.f, 0.f, 0.f};
    }
    return {v.x / L, v.y / L, v.z / L};
}

// Map linear sticker index to (row, t) in the1+3+5+7 tetractys layout (apex at face corner 0).
void slotToRowT(int slot, int& row, int& t) {
    int idx = 0;
    for (int r = 0; r < Tetrahedron::kSubdivision; ++r) {
        const int count = 2 * r + 1;
        if (slot < idx + count) {
            row = r;
            t = slot - idx;
            return;
        }
        idx += count;
    }
    row = 0;
    t = 0;
}

} // namespace

const std::array<std::array<int, 3>, Tetrahedron::kFaces>& Tetrahedron::faceCornerVertices() {
    // Face i is opposite body vertex i; corners are the other three vertices in CCW order from outside.
    static const std::array<std::array<int, 3>, kFaces> kCorners = {{
        {{1, 2, 3}},
        {{0, 2, 3}},
        {{0, 1, 3}},
        {{0, 1, 2}},
    }};
    return kCorners;
}

sf::Vector3f Tetrahedron::bodyVertexPosition(int bodyVertex) {
    // Regular tetrahedron vertices around origin (unnormalized; axis from origin matches vertex direction).
    switch (bodyVertex) {
        case 0:
            return {1.f, 1.f, 1.f};
        case 1:
            return {1.f, -1.f, -1.f};
        case 2:
            return {-1.f, 1.f, -1.f};
        case 3:
            return {-1.f, -1.f, 1.f};
        default:
            return {0.f, 0.f, 0.f};
    }
}

Tetrahedron::Tetrahedron() { reset(); }

// Restores every sticker on face f to color index f (solved configuration).
void Tetrahedron::reset() {
    for (int f = 0; f < kFaces; ++f) {
        for (int s = 0; s < kStickersPerFace; ++s) {
            faces_[static_cast<std::size_t>(f)][static_cast<std::size_t>(s)] = f;
        }
    }
}

// Builds per-face sticker centers and barycentric weights once (lazy, thread-safe via const).
void Tetrahedron::ensureGeometry() const {
    if (geom_.built) {
        return;
    }

    const float scale = 1.8f;
    const auto& corners = faceCornerVertices();

    for (int face = 0; face < kFaces; ++face) {
        sf::Vector3f P0 = bodyVertexPosition(corners[static_cast<std::size_t>(face)][0]);
        sf::Vector3f P1 = bodyVertexPosition(corners[static_cast<std::size_t>(face)][1]);
        sf::Vector3f P2 = bodyVertexPosition(corners[static_cast<std::size_t>(face)][2]);
        P0 = {P0.x * scale, P0.y * scale, P0.z * scale};
        P1 = {P1.x * scale, P1.y * scale, P1.z * scale};
        P2 = {P2.x * scale, P2.y * scale, P2.z * scale};

        const sf::Vector3f e1 = P1 - P0;
        const sf::Vector3f e2 = P2 - P0;
        sf::Vector3f n = cross3(e1, e2);
        const sf::Vector3f oppVtx = bodyVertexPosition(face);
        const sf::Vector3f oppPos{oppVtx.x * scale, oppVtx.y * scale, oppVtx.z * scale};
        const sf::Vector3f fc{(P0.x + P1.x + P2.x) / 3.f, (P0.y + P1.y + P2.y) / 3.f,
                                (P0.z + P1.z + P2.z) / 3.f};
        if (dot3(n, oppPos - fc) < 0.f) {
            std::swap(P1, P2);
        }

        const int nSub = kSubdivision;
        int slot = 0;
        for (int r = 0; r < nSub; ++r) {
            const float u0 = 1.f - (static_cast<float>(r) + 0.5f) / static_cast<float>(nSub);
            const float sum12 = 1.f - u0;
            const int rowCount = 2 * r + 1;
            for (int t = 0; t < rowCount; ++t) {
                const float u1 = (static_cast<float>(t) + 0.5f) / static_cast<float>(rowCount) * sum12;
                const float u2 = sum12 - u1;
                const sf::Vector3f c{P0.x * u0 + P1.x * u1 + P2.x * u2, P0.y * u0 + P1.y * u1 + P2.y * u2,
                                        P0.z * u0 + P1.z * u1 + P2.z * u2};
                geom_.centers[static_cast<std::size_t>(face)][static_cast<std::size_t>(slot)] = c;
                geom_.bary[static_cast<std::size_t>(face)][static_cast<std::size_t>(slot)] = {u0, u1, u2};
                ++slot;
            }
        }
    }

    geom_.built = true;
}

int Tetrahedron::flatIndex(int face, int slot) { return face * kStickersPerFace + slot; }

void Tetrahedron::unflatIndex(int flat, int& face, int& slot) {
    face = flat / kStickersPerFace;
    slot = flat % kStickersPerFace;
}

// Returns true for upward-pointing micro-triangles in the tetractys row pattern (slot index).
bool Tetrahedron::stickerIsUpTriangle(int slot) {
    int row = 0;
    int t = 0;
    slotToRowT(slot, row, t);
    return (t % 2) == 0;
}

sf::Vector3f Tetrahedron::rotateAroundAxis(const sf::Vector3f& p, const sf::Vector3f& axisUnit,
 float radians) const {
    const sf::Vector3f k = axisUnit;
    const float cosT = std::cos(radians);
    const float sinT = std::sin(radians);
    const sf::Vector3f v = p;
    const float kdotv = dot3(k, v);
    const sf::Vector3f kxv = cross3(k, v);
    return {v.x * cosT + k.x * (kdotv * (1.f - cosT)) + kxv.x * sinT,
            v.y * cosT + k.y * (kdotv * (1.f - cosT)) + kxv.y * sinT,
            v.z * cosT + k.z * (kdotv * (1.f - cosT)) + kxv.z * sinT};
}

// Finds the closest sticker center in 3D (used to realize a rigid 120-degree cap rotation).
int Tetrahedron::findNearestSticker(const sf::Vector3f& world) const {
    ensureGeometry();
    int best = 0;
    float bestD = std::numeric_limits<float>::max();
    for (int f = 0; f < kFaces; ++f) {
        for (int s = 0; s < kStickersPerFace; ++s) {
            const sf::Vector3f c = geom_.centers[static_cast<std::size_t>(f)][static_cast<std::size_t>(s)];
            const float dx = c.x - world.x;
            const float dy = c.y - world.y;
            const float dz = c.z - world.z;
            const float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 < bestD) {
                bestD = d2;
                best = flatIndex(f, s);
            }
        }
    }
    return best;
}

sf::Vector3f Tetrahedron::stickerCenter(int face, int slot) const {
    ensureGeometry();
    return geom_.centers[static_cast<std::size_t>(face)][static_cast<std::size_t>(slot)];
}

// True if this sticker lies in the first `depth` barycentric rows from `vertex` on `face`.
bool Tetrahedron::stickerInCap(int face, int slot, int vertex, int depth) const {
    ensureGeometry();
    if (depth < 1 || depth > kSubdivision) {
        return false;
    }
    const auto& fc = faceCornerVertices()[static_cast<std::size_t>(face)];
    int local = -1;
    for (int i = 0; i < 3; ++i) {
        if (fc[static_cast<std::size_t>(i)] == vertex) {
            local = i;
            break;
        }
    }
    if (local < 0) {
        return false;
    }
    const float w =
        geom_.bary[static_cast<std::size_t>(face)][static_cast<std::size_t>(slot)][static_cast<std::size_t>(local)];
    const int row = std::clamp(static_cast<int>(std::floor((1.f - w) * static_cast<float>(kSubdivision))), 0,
 kSubdivision - 1);
    return row < depth;
}

// Applies a 120-degree rotation of the cap at `vertex` and `depth`, permuting sticker colors.
void Tetrahedron::rotate(int vertex, int depth, bool ccw) {
    if (vertex < 0 || vertex > 3 || depth < 1 || depth > 3) {
        return;
    }
    ensureGeometry();

    const sf::Vector3f axis = norm3(bodyVertexPosition(vertex));
    const float angle = ccw ? (2.f * static_cast<float>(M_PI) / 3.f) : (-2.f * static_cast<float>(M_PI) / 3.f);

    std::array<int, kFaces * kStickersPerFace> oldFlat{};
    std::array<int, kFaces * kStickersPerFace> newFlat{};
    const int total = kFaces * kStickersPerFace;
    for (int i = 0; i < total; ++i) {
        int f = 0;
        int s = 0;
        unflatIndex(i, f, s);
        oldFlat[static_cast<std::size_t>(i)] = faces_[static_cast<std::size_t>(f)][static_cast<std::size_t>(s)];
        newFlat[static_cast<std::size_t>(i)] = oldFlat[static_cast<std::size_t>(i)];
    }

    std::vector<int> capFlat;
    capFlat.reserve(64);
    for (int f = 0; f < kFaces; ++f) {
        for (int s = 0; s < kStickersPerFace; ++s) {
            if (stickerInCap(f, s, vertex, depth)) {
                capFlat.push_back(flatIndex(f, s));
            }
        }
    }

    for (int srcFlatIdx : capFlat) {
        int sf = 0;
        int ss = 0;
        unflatIndex(srcFlatIdx, sf, ss);
        const sf::Vector3f p = geom_.centers[static_cast<std::size_t>(sf)][static_cast<std::size_t>(ss)];
        const sf::Vector3f p2 = rotateAroundAxis(p, axis, angle);
        const int dstFlatIdx = findNearestSticker(p2);
        newFlat[static_cast<std::size_t>(dstFlatIdx)] = oldFlat[static_cast<std::size_t>(srcFlatIdx)];
    }

    for (int i = 0; i < total; ++i) {
        int f = 0;
        int s = 0;
        unflatIndex(i, f, s);
        faces_[static_cast<std::size_t>(f)][static_cast<std::size_t>(s)] = newFlat[static_cast<std::size_t>(i)];
    }
}

// Parses a compact token "vd" or "vd'" with v=vertex digit, d=depth digit; optional ' = ccw.
bool Tetrahedron::applyMoveToken(const std::string& token) {
    if (token.size() < 2) {
        return false;
    }
    int v = token[0] - '0';
    int d = token[1] - '0';
    bool ccw = false;
    if (token.size() >= 3 && token[2] == '\'') {
        ccw = true;
    }
    if (v < 0 || v > 3 || d < 1 || d > 3) {
        return false;
    }
    rotate(v, d, ccw);
    return true;
}

// Applies `numMoves` random legal twists (uniform vertex, depth, and direction).
void Tetrahedron::scramble(int numMoves) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (int i = 0; i < numMoves; ++i) {
        const int v = std::rand() % 4;
        const int d = 1 + (std::rand() % 3);
        const bool ccw = (std::rand() % 2) == 0;
        rotate(v, d, ccw);
    }
}

// Solved when each macro face is a single solid color across all 16 stickers.
bool Tetrahedron::isSolved() const {
    for (int f = 0; f < kFaces; ++f) {
        const int c = faces_[static_cast<std::size_t>(f)][0];
        for (int s = 1; s < kStickersPerFace; ++s) {
            if (faces_[static_cast<std::size_t>(f)][static_cast<std::size_t>(s)] != c) {
                return false;
            }
        }
    }
    return true;
}
