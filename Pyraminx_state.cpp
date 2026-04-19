// Build permutations: vertex moves act on one tetractys band only; edge moves unchanged.

#include "pyraminx_state.h"

#include "turn_geometry.h"

#include <cmath>
#include <functional>
#include <random>
#include <vector>

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kVertexFullRad = 2.f * kPi / 3.f;

/**
 * Minimum-cost perfect matching for a square cost matrix (Hungarian / Kuhn–Munkres).
 * Row i is assigned column match[i]; minimizes sum_i cost[i][match[i]].
 * Based on the O(n³) formulation on Algorithms for Competitive Programming (e-maxx / cp-algorithms).
 */
std::vector<int> hungarianSquareMin(const std::vector<std::vector<double>>& a) {
    const int n = static_cast<int>(a.size());
    if (n == 0) {
        return {};
    }
    const int m = n;
    const double kInf = 1e100;
    std::vector<double> u(static_cast<std::size_t>(n + 1));
    std::vector<double> v(static_cast<std::size_t>(m + 1));
    std::vector<int> p(static_cast<std::size_t>(m + 1));
    std::vector<int> way(static_cast<std::size_t>(m + 1));
    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<double> minv(static_cast<std::size_t>(m + 1), kInf);
        std::vector<char> used(static_cast<std::size_t>(m + 1), 0);
        do {
            used[static_cast<std::size_t>(j0)] = 1;
            const int i0 = p[static_cast<std::size_t>(j0)];
            double delta = kInf;
            int j1 = 0;
            for (int j = 1; j <= m; ++j) {
                if (!used[static_cast<std::size_t>(j)]) {
                    const double cur = a[static_cast<std::size_t>(i0 - 1)][static_cast<std::size_t>(j - 1)] -
                                       u[static_cast<std::size_t>(i0)] - v[static_cast<std::size_t>(j)];
                    if (cur < minv[static_cast<std::size_t>(j)]) {
                        minv[static_cast<std::size_t>(j)] = cur;
                        way[static_cast<std::size_t>(j)] = j0;
                    }
                    if (minv[static_cast<std::size_t>(j)] < delta) {
                        delta = minv[static_cast<std::size_t>(j)];
                        j1 = j;
                    }
                }
            }
            for (int j = 0; j <= m; ++j) {
                if (used[static_cast<std::size_t>(j)]) {
                    u[static_cast<std::size_t>(p[static_cast<std::size_t>(j)])] += delta;
                    v[static_cast<std::size_t>(j)] -= delta;
                } else {
                    minv[static_cast<std::size_t>(j)] -= delta;
                }
            }
            j0 = j1;
        } while (p[static_cast<std::size_t>(j0)] != 0);
        do {
            const int j1 = way[static_cast<std::size_t>(j0)];
            p[static_cast<std::size_t>(j0)] = p[static_cast<std::size_t>(j1)];
            j0 = j1;
        } while (j0 != 0);
    }
    std::vector<int> colForRow(static_cast<std::size_t>(n), 0);
    for (int j = 1; j <= m; ++j) {
        if (p[static_cast<std::size_t>(j)] != 0) {
            colForRow[static_cast<std::size_t>(p[static_cast<std::size_t>(j)] - 1)] = j - 1;
        }
    }
    return colForRow;
}

/**
 * UX: n×n cost matrix for Hungarian matching — squared distance from mapped centroid i to mesh centroid j
 * (both restricted to `domain` sticker indices).
 */
std::vector<std::vector<double>> buildCentroidCostMatrix(const TetMesh& mesh, const std::vector<int>& domain,
                                                          const std::function<Vec3(Vec3)>& mapPoint) {
    const int n = static_cast<int>(domain.size());
    std::vector<std::vector<double>> cost(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        cost[static_cast<std::size_t>(i)].resize(static_cast<std::size_t>(n));
        const Vec3 dst =
            mapPoint(mesh.centroids[static_cast<std::size_t>(domain[static_cast<std::size_t>(i)])]);
        for (int j = 0; j < n; ++j) {
            const Vec3 q = mesh.centroids[static_cast<std::size_t>(domain[static_cast<std::size_t>(j)])];
            const float dx = dst.x - q.x;
            const float dy = dst.y - q.y;
            const float dz = dst.z - q.z;
            cost[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                static_cast<double>(dx * dx + dy * dy + dz * dz);
        }
    }
    return cost;
}

/** UX: build identity permutation, then replace entries for `domain` using minimum-cost perfect matching. */
Permutation buildPermutation(const TetMesh& mesh, const std::vector<int>& domain,
                             const std::function<Vec3(Vec3)>& mapPoint) {
    Permutation p{};
    for (int i = 0; i < kStickerCount; ++i) {
        p[static_cast<std::size_t>(i)] = i;
    }
    if (domain.empty()) {
        return p;
    }
    const std::vector<std::vector<double>> cost = buildCentroidCostMatrix(mesh, domain, mapPoint);
    const std::vector<int> match = hungarianSquareMin(cost);
    const int dn = static_cast<int>(domain.size());
    for (int i = 0; i < dn; ++i) {
        const int j = match[static_cast<std::size_t>(i)];
        const int newIdx = domain[static_cast<std::size_t>(j)];
        const int oldIdx = domain[static_cast<std::size_t>(i)];
        p[static_cast<std::size_t>(newIdx)] = oldIdx;
    }
    return p;
}

} // namespace

std::array<std::array<std::array<Permutation, 2>, kVertexBands>, 4> PyraminxState::vertexLayerPerms_{};
std::array<Permutation, 6> PyraminxState::edgePerms_{};
bool PyraminxState::permsReady_ = false;

void PyraminxState::buildPermsFromMesh(const TetMesh& mesh) {
    if (permsReady_) {
        return;
    }
    std::vector<int> neigh;
    for (int v = 0; v < 4; ++v) {
        for (int layer = 0; layer < kVertexBands; ++layer) {
            vertexLayerTurnStickerIndices(mesh, v, layer, neigh);
            for (int s : {1, -1}) {
                auto mapPt = [&](Vec3 p) { return vertexTurnPosition(mesh, p, v, s, kVertexFullRad); };
                const Permutation p = buildPermutation(mesh, neigh, mapPt);
                vertexLayerPerms_[static_cast<std::size_t>(v)][static_cast<std::size_t>(layer)][s == 1 ? 1 : 0] =
                    p;
            }
        }
    }

    for (int e = 0; e < 6; ++e) {
        edgeTurnStickerIndices(mesh, e, neigh);
        auto mapPt = [&](Vec3 p) { return edgeTurnPosition(mesh, p, e, kPi); };
        const Permutation p = buildPermutation(mesh, neigh, mapPt);
        edgePerms_[static_cast<std::size_t>(e)] = p;
    }

    permsReady_ = true;
}

Permutation PyraminxState::invertPerm(const Permutation& p) {
    Permutation inv{};
    for (int i = 0; i < kStickerCount; ++i) {
        inv[static_cast<std::size_t>(p[static_cast<std::size_t>(i)])] = i;
    }
    return inv;
}

void PyraminxState::applyPermInPlace(std::array<int, kStickerCount>& colors, const Permutation& p) {
    std::array<int, kStickerCount> next{};
    for (int i = 0; i < kStickerCount; ++i) {
        next[static_cast<std::size_t>(i)] = colors[static_cast<std::size_t>(p[static_cast<std::size_t>(i)])];
    }
    colors = next;
}

PyraminxState::PyraminxState(const TetMesh& mesh) {
    buildPermsFromMesh(mesh);
    reset();
}

void PyraminxState::reset() {
    for (int i = 0; i < kStickerCount; ++i) {
        const int f = i / kTrisPerFace;
        color_[static_cast<std::size_t>(i)] = f;
    }
    history_.clear();
}

void PyraminxState::apply(const PuzzleMove& m) {
    Permutation p{};
    if (m.kind == PuzzleMove::Kind::VertexLayer) {
        const int dirIdx = m.dir >= 0 ? 1 : 0;
        const int L = m.layer < kVertexBands ? m.layer : 0;
        p = vertexLayerPerms_[static_cast<std::size_t>(m.index)][static_cast<std::size_t>(L)]
                [static_cast<std::size_t>(dirIdx)];
    } else {
        p = edgePerms_[static_cast<std::size_t>(m.index)];
    }
    applyPermInPlace(color_, p);
    history_.push_back(m);
}

void PyraminxState::autoComplete() {
    // Jump to canonical solved coloring (each face uniform); do not rely on undo chain correctness.
    reset();
}

void PyraminxState::undo() {
    if (history_.empty()) {
        return;
    }
    const PuzzleMove m = history_.back();
    history_.pop_back();
    Permutation p{};
    if (m.kind == PuzzleMove::Kind::VertexLayer) {
        const int dirIdx = m.dir >= 0 ? 1 : 0;
        const int L = m.layer < kVertexBands ? m.layer : 0;
        p = vertexLayerPerms_[static_cast<std::size_t>(m.index)][static_cast<std::size_t>(L)]
                [static_cast<std::size_t>(dirIdx)];
    } else {
        p = edgePerms_[static_cast<std::size_t>(m.index)];
    }
    const Permutation inv = invertPerm(p);
    applyPermInPlace(color_, inv);
}

void PyraminxState::scramble(int moves, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> vPick(0, 3);
    std::uniform_int_distribution<int> layerPick(0, kVertexBands - 1);
    std::uniform_int_distribution<int> ePick(0, 5);
    std::uniform_int_distribution<int> dirPick(0, 1);
    for (int i = 0; i < moves; ++i) {
        if ((rng() & 1) == 0) {
            PuzzleMove m{PuzzleMove::Kind::VertexLayer, static_cast<std::uint8_t>(vPick(rng)),
                         static_cast<std::uint8_t>(layerPick(rng)),
                         static_cast<std::int8_t>(dirPick(rng) == 0 ? -1 : 1)};
            apply(m);
        } else {
            PuzzleMove m{PuzzleMove::Kind::Edge, static_cast<std::uint8_t>(ePick(rng)), 0, 1};
            apply(m);
        }
    }
}
