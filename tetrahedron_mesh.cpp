// Build regular tetrahedron with per-face order-6 barycentric lattice (28 lattice points, 36 tris per face).

#include "tetrahedron_mesh.h"

#include <cassert>
#include <vector>

namespace {

/** Row-major index for lattice point (a,b,c) with a+b+c=n (matches `latticePoints` order). */
int latticeLinearIndex(int n, int a, int b) {
    int id = 0;
    for (int aa = 0; aa < a; ++aa) {
        id += (n - aa + 1);
    }
    return id + b;
}

/**
 * Fills `out` with the n² small triangles: for each (a,b,c) with a+b+c=n, one “up” triangle
 * when c≥1 and one “down” triangle when c≥2 — standard staggered triangular grid connectivity.
 */
void appendLatticeTriangles(int n, std::vector<std::array<int, 3>>& out) {
    auto idx = [n](int a, int b) { return latticeLinearIndex(n, a, b); };
    for (int a = 0; a <= n; ++a) {
        for (int b = 0; b <= n - a; ++b) {
            const int c = n - a - b;
            if (c >= 1) {
                out.push_back({idx(a, b), idx(a, b + 1), idx(a + 1, b)});
            }
            if (c >= 2) {
                out.push_back({idx(a, b + 1), idx(a + 1, b), idx(a + 1, b + 1)});
            }
        }
    }
}

/** Corners of a regular tetrahedron centered at origin (unnormalized). */
std::array<Vec3, 4> rawCorners() {
    return {Vec3{1.f, 1.f, 1.f}, Vec3{1.f, -1.f, -1.f}, Vec3{-1.f, 1.f, -1.f}, Vec3{-1.f, -1.f, 1.f}};
}

/** Face f: three corner indices (CCW when viewed from outside — fixed by cross product check). */
void assignFaceCorners(std::array<std::array<int, 3>, kFaces>& out) {
    out[0] = {0, 2, 1};
    out[1] = {0, 1, 3};
    out[2] = {0, 3, 2};
    out[3] = {1, 2, 3};
}

/** Barycentric lattice points (a+b+c=n), same order as `latticeLinearIndex`. */
void latticePoints(int n, Vec3 p0, Vec3 p1, Vec3 p2, std::vector<Vec3>& outPts) {
    outPts.clear();
    for (int a = 0; a <= n; ++a) {
        for (int b = 0; b <= n - a; ++b) {
            const int c = n - a - b;
            const Vec3 p = (p0 * static_cast<float>(a) + p1 * static_cast<float>(b) + p2 * static_cast<float>(c)) *
                           (1.0f / static_cast<float>(n));
            outPts.push_back(p);
        }
    }
}

Vec3 triangleNormalOutward(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 faceOut) {
    Vec3 n = cross(p1 - p0, p2 - p0);
    if (dot(n, faceOut) < 0.f) {
        std::swap(p1, p2);
        n = cross(p1 - p0, p2 - p0);
    }
    return normalize(n);
}

/**
 * UX: fill mesh stickers for face index `f` — lattice points, micro-triangle indices, outward normals, centroids.
 * triIdx and lattice must match order from appendLatticeTriangles / latticePoints.
 */
void buildStickersForFace(int f, TetMesh& mesh, const std::vector<std::array<int, 3>>& triIdx, std::vector<Vec3>& lattice) {
    const int i0 = mesh.faceCorners[f][0];
    const int i1 = mesh.faceCorners[f][1];
    const int i2 = mesh.faceCorners[f][2];
    const Vec3 p0 = mesh.corners[i0];
    const Vec3 p1 = mesh.corners[i1];
    const Vec3 p2 = mesh.corners[i2];
    const Vec3 faceCenter = (p0 + p1 + p2) * (1.f / 3.f);
    const Vec3 faceOut = normalize(faceCenter);

    latticePoints(kLatticeOrder, p0, p1, p2, lattice);

    for (int t = 0; t < kTrisPerFace; ++t) {
        const int g = f * kTrisPerFace + t;
        const int ia = triIdx[static_cast<std::size_t>(t)][0];
        const int ib = triIdx[static_cast<std::size_t>(t)][1];
        const int ic = triIdx[static_cast<std::size_t>(t)][2];
        Vec3 va = lattice[static_cast<std::size_t>(ia)];
        Vec3 vb = lattice[static_cast<std::size_t>(ib)];
        Vec3 vc = lattice[static_cast<std::size_t>(ic)];
        Vec3 nrm = triangleNormalOutward(va, vb, vc, faceOut);
        if (dot(nrm, faceOut) <= 0.f) {
            std::swap(vb, vc);
            nrm = triangleNormalOutward(va, vb, vc, faceOut);
        }
        mesh.triangles[static_cast<std::size_t>(g)].pos = {va, vb, vc};
        mesh.triangles[static_cast<std::size_t>(g)].normal = nrm;
        mesh.centroids[static_cast<std::size_t>(g)] = (va + vb + vc) * (1.f / 3.f);
    }
}

} // namespace

TetMesh buildTetrahedronMesh(float edgeLength) {
    TetMesh mesh{};
    assignFaceCorners(mesh.faceCorners);

    std::array<Vec3, 4> raw = rawCorners();
    const float rawEdge = length(raw[1] - raw[0]);
    const float scale = edgeLength / rawEdge;
    for (int i = 0; i < 4; ++i) {
        mesh.corners[i] = raw[i] * scale;
    }

    const int n = kLatticeOrder;

    std::vector<std::array<int, 3>> triIdx;
    triIdx.reserve(static_cast<std::size_t>(n * n));
    appendLatticeTriangles(n, triIdx);
    assert(triIdx.size() == static_cast<std::size_t>(n * n));

    std::vector<Vec3> lattice;

    for (int f = 0; f < kFaces; ++f) {
        buildStickersForFace(f, mesh, triIdx, lattice);
    }

    return mesh;
}
