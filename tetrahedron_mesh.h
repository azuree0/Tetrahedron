// Regular tetrahedron surface with order-6 tetractys-style triangular subdivision per face.

#pragma once

#include "math3.h"

#include <array>

/** Barycentric lattice order per face edge (6 segments → 6² = 36 micro-triangles per face). */
inline constexpr int kLatticeOrder = 6;

/** Vertex-turn bands along pivot barycentric weight (tip → base); keys 1–6 map to these. */
inline constexpr int kVertexBands = 6;

/** Global sticker / micro-triangle count: 4 faces * 36 elementary triangles. */
inline constexpr int kFaces = 4;
inline constexpr int kTrisPerFace = kLatticeOrder * kLatticeOrder;
inline constexpr int kStickerCount = kFaces * kTrisPerFace;

/** One rendered triangle: three positions and outward unit normal. */
struct MeshTriangle {
    std::array<Vec3, 3> pos{};
    Vec3 normal{};
};

/** Full mesh for rendering and move permutation matching. */
struct TetMesh {
    /** Tetrahedron corner positions (centered at origin), length ~1 edge after scale. */
    std::array<Vec3, 4> corners{};

    /** Per sticker: centroid used for permutation search. */
    std::array<Vec3, kStickerCount> centroids{};

    /** Geometry for OpenGL. */
    std::array<MeshTriangle, kStickerCount> triangles{};

    /** faceCorners[f][0..2] = corner indices into `corners` for that face (CCW outward). */
    std::array<std::array<int, 3>, kFaces> faceCorners{};
};

/** Builds corners, subdivides each face into 36 same-orientation micro-triangles (order-6 lattice). */
TetMesh buildTetrahedronMesh(float edgeLength);
