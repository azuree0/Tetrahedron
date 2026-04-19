// Sticker sets and partial rigid transforms for vertex / edge turns (shared by state + renderer).

#pragma once

#include "math3.h"
#include "tetrahedron_mesh.h"

#include <vector>

/** Per-face tetractys bands from a corner: 0 = tip (top), 5 = base strip (six equal depth bins). */
int tetractysLayerFromVertex(const TetMesh& mesh, int stickerIndex, int cornerVertex);

/**
 * UX: vertex + layer for a vertex turn that moves the sticker’s tetractys band, using the face’s tip
 * (faceCorners[f][0]) as pivot — matches mesh lattice construction.
 */
bool stickerIndexToVertexTurn(const TetMesh& mesh, int stickerIndex, int& outVertex, int& outLayer);

/**
 * Stickers in that band on the three faces meeting at `v` (layer index matches band 0..5).
 * Outermost band (layer kVertexBands-1): also includes every sticker on the face opposite `v`, so the
 * base cap turns rigidly with the same 120° twist.
 */
void vertexLayerTurnStickerIndices(const TetMesh& mesh, int v, int layer, std::vector<int>& out);

/** Stickers on the two faces sharing edge `edgeIndex` 0..5 — pairs (0,1)(0,2)(0,3)(1,2)(1,3)(2,3). */
void edgeTurnStickerIndices(const TetMesh& mesh, int edgeIndex, std::vector<int>& out);

/** Endpoints of edge index 0..5. */
void edgeEndpoints(int edgeIndex, int& a, int& b);

/** Partial vertex turn: angleRad in [0, 2π/3]; dir ±1 selects rotation sense around center→vertex axis. */
Vec3 vertexTurnPosition(const TetMesh& mesh, Vec3 p, int v, int dir, float angleRad);

Vec3 vertexTurnNormal(const TetMesh& mesh, Vec3 n, int v, int dir, float angleRad);

/** Partial edge turn: angleRad in [0, π]. */
Vec3 edgeTurnPosition(const TetMesh& mesh, Vec3 p, int edgeIndex, float angleRad);

Vec3 edgeTurnNormal(const TetMesh& mesh, Vec3 n, int edgeIndex, float angleRad);
