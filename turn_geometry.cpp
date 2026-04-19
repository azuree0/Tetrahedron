// Geometry helpers: tetractys layers from barycentric depth, then rigid transforms.

#include "turn_geometry.h"

#include <cmath>

namespace {

bool faceHasCorner(const TetMesh& mesh, int f, int c) {
    for (int k = 0; k < 3; ++k) {
        if (mesh.faceCorners[static_cast<std::size_t>(f)][static_cast<std::size_t>(k)] == c) {
            return true;
        }
    }
    return false;
}

/** Unique face whose three corners are all different from `cornerVertex` (the base opposite that apex). */
int faceOppositeVertex(const TetMesh& mesh, int cornerVertex) {
    for (int f = 0; f < kFaces; ++f) {
        if (!faceHasCorner(mesh, f, cornerVertex)) {
            return f;
        }
    }
    return -1;
}

/** Map pivot barycentric weight u to band 0..kVertexBands-1 (0 = tip, 5 = base). */
int layerFromPivotWeight(float u) {
    constexpr float eps = 2e-4f;
    if (u >= 5.f / 6.f - eps) {
        return 0;
    }
    if (u >= 4.f / 6.f - eps) {
        return 1;
    }
    if (u >= 3.f / 6.f - eps) {
        return 2;
    }
    if (u >= 2.f / 6.f - eps) {
        return 3;
    }
    if (u >= 1.f / 6.f - eps) {
        return 4;
    }
    return 5;
}

} // namespace

int tetractysLayerFromVertex(const TetMesh& mesh, int stickerIndex, int cornerVertex) {
    const int f = stickerIndex / kTrisPerFace;
    if (!faceHasCorner(mesh, f, cornerVertex)) {
        return -1;
    }
    Vec3 c0 = mesh.corners[static_cast<std::size_t>(cornerVertex)];
    Vec3 c1{};
    Vec3 c2{};
    int filled = 0;
    for (int k = 0; k < 3; ++k) {
        const int cid = mesh.faceCorners[static_cast<std::size_t>(f)][static_cast<std::size_t>(k)];
        if (cid == cornerVertex) {
            continue;
        }
        if (filled == 0) {
            c1 = mesh.corners[static_cast<std::size_t>(cid)];
        } else {
            c2 = mesh.corners[static_cast<std::size_t>(cid)];
        }
        ++filled;
    }
    if (filled != 2) {
        return -1;
    }
    float u = 0.f;
    float bv = 0.f;
    float bw = 0.f;
    barycentric3(mesh.centroids[static_cast<std::size_t>(stickerIndex)], c0, c1, c2, u, bv, bw);
    return layerFromPivotWeight(u);
}

void vertexLayerTurnStickerIndices(const TetMesh& mesh, int v, int layer, std::vector<int>& out) {
    out.clear();
    for (int s = 0; s < kStickerCount; ++s) {
        if (tetractysLayerFromVertex(mesh, s, v) == layer) {
            out.push_back(s);
        }
    }
    // Outermost band: the triangular face opposite apex `v` is invariant under the same 120° twist — include it.
    if (layer == kVertexBands - 1) {
        const int fo = faceOppositeVertex(mesh, v);
        if (fo >= 0) {
            for (int t = 0; t < kTrisPerFace; ++t) {
                out.push_back(fo * kTrisPerFace + t);
            }
        }
    }
}

void edgeEndpoints(int edgeIndex, int& a, int& b) {
    static const int ka[6] = {0, 0, 0, 1, 1, 2};
    static const int kb[6] = {1, 2, 3, 2, 3, 3};
    a = ka[edgeIndex];
    b = kb[edgeIndex];
}

/**
 * UX: the two face indices that share both endpoints of the edge (tetrahedron has two faces per edge).
 * Returns how many faces were found (0, 1, or 2); f0/f1 are -1 when unused.
 */
void findFacesIncidentToEdge(const TetMesh& mesh, int cornerA, int cornerB, int& f0, int& f1, int& foundCount) {
    f0 = -1;
    f1 = -1;
    foundCount = 0;
    for (int f = 0; f < kFaces; ++f) {
        const bool ha = faceHasCorner(mesh, f, cornerA);
        const bool hb = faceHasCorner(mesh, f, cornerB);
        if (ha && hb) {
            if (foundCount == 0) {
                f0 = f;
            } else {
                f1 = f;
            }
            ++foundCount;
        }
    }
}

/** UX: append all sticker indices on face `f` (full order-6 lattice) to `out`. */
void appendAllStickersOnFace(int faceIndex, std::vector<int>& out) {
    for (int t = 0; t < kTrisPerFace; ++t) {
        out.push_back(faceIndex * kTrisPerFace + t);
    }
}

void edgeTurnStickerIndices(const TetMesh& mesh, int edgeIndex, std::vector<int>& out) {
    int a = 0;
    int b = 0;
    edgeEndpoints(edgeIndex, a, b);
    out.clear();
    int f0 = -1;
    int f1 = -1;
    int found = 0;
    findFacesIncidentToEdge(mesh, a, b, f0, f1, found);
    if (f0 >= 0) {
        appendAllStickersOnFace(f0, out);
    }
    if (f1 >= 0 && f1 != f0) {
        appendAllStickersOnFace(f1, out);
    }
}

Vec3 vertexTurnPosition(const TetMesh& mesh, Vec3 p, int v, int dir, float angleRad) {
    const Vec3 axis = normalize(mesh.corners[static_cast<std::size_t>(v)]);
    const float s = dir >= 0 ? 1.f : -1.f;
    return rotateAroundAxis(p, axis, s * angleRad);
}

Vec3 vertexTurnNormal(const TetMesh& mesh, Vec3 n, int v, int dir, float angleRad) {
    return vertexTurnPosition(mesh, n, v, dir, angleRad);
}

Vec3 edgeTurnPosition(const TetMesh& mesh, Vec3 p, int edgeIndex, float angleRad) {
    int a = 0;
    int b = 0;
    edgeEndpoints(edgeIndex, a, b);
    const Vec3 vi = mesh.corners[static_cast<std::size_t>(a)];
    const Vec3 vj = mesh.corners[static_cast<std::size_t>(b)];
    const Vec3 mid = (vi + vj) * 0.5f;
    const Vec3 axis = normalize(vj - vi);
    return rotatePointAroundAxis(p, mid, axis, angleRad);
}

Vec3 edgeTurnNormal(const TetMesh& mesh, Vec3 n, int edgeIndex, float angleRad) {
    int a = 0;
    int b = 0;
    edgeEndpoints(edgeIndex, a, b);
    const Vec3 vi = mesh.corners[static_cast<std::size_t>(a)];
    const Vec3 vj = mesh.corners[static_cast<std::size_t>(b)];
    const Vec3 axis = normalize(vj - vi);
    return rotateAroundAxis(n, axis, angleRad);
}
