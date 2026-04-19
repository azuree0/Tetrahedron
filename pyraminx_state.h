// Sticker colors: tetractys vertex-layer turns and edge moves with undo.

#pragma once

#include "tetrahedron_mesh.h"

#include <array>
#include <cstdint>
#include <vector>

/** Move: vertex twist affects one tetractys band (0=tip … kVertexBands-1=base) at a corner; edge unchanged. */
struct PuzzleMove {
    enum class Kind : std::uint8_t { VertexLayer = 0, Edge = 1 };
    Kind kind = Kind::VertexLayer;
    std::uint8_t index = 0;  // vertex 0..3 or edge 0..5
    std::uint8_t layer = 0;  // 0..kVertexBands-1 for vertex bands; ignored for Edge
    std::int8_t dir = 1;     // vertex: ±1 for ±120°; edge: ignored (180°)
};

/** Permutation of kStickerCount stickers; perm[i] = old index that moves to slot i after move. */
using Permutation = std::array<int, kStickerCount>;

class PyraminxState {
public:
    explicit PyraminxState(const TetMesh& mesh);

    /** Solved: face f has color f on every sticker of that face. */
    void reset();

    /** Apply one move to sticker colors and append it to undo history. */
    void apply(const PuzzleMove& m);

    /** Undo the last applied move (inverse permutation on colors). */
    void undo();

    /** Solved state: each face one uniform color (face index 0..3); clears history. */
    void autoComplete();

    /** Apply `moves` random vertex or edge moves using `seed` for RNG. */
    void scramble(int moves, unsigned seed);

    /** Current color index per sticker slot (face palette 0..3). */
    const std::array<int, kStickerCount>& colors() const { return color_; }

private:
    /** Precompute Hungarian-derived permutations from mesh centroids (once per process). */
    static void buildPermsFromMesh(const TetMesh& mesh);

    /** Inverse permutation: inv[p[i]] = i. */
    static Permutation invertPerm(const Permutation& p);

    /** perm[i] = old sticker index now at slot i — rewrites `colors` in place. */
    static void applyPermInPlace(std::array<int, kStickerCount>& colors, const Permutation& p);

    std::array<int, kStickerCount> color_{};
    std::vector<PuzzleMove> history_;

    /** [vertex][tetractys layer 0..kVertexBands-1][dir 0=-1, 1=+1] */
    static std::array<std::array<std::array<Permutation, 2>, kVertexBands>, 4> vertexLayerPerms_;
    static std::array<Permutation, 6> edgePerms_;
    static bool permsReady_;
};
