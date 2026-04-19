// Tetrahedron puzzle: Master Pyraminx-style sticker model (4 faces x 16 triangles).
// Rotations are 120 degrees about body vertices; depth selects how many barycentric rows move.

#ifndef TETRAHEDRON_H
#define TETRAHEDRON_H

#include <array>
#include <string>
#include <vector>

#include <SFML/System/Vector3.hpp>

// Solved-state face colors (one base color per macro face).
enum class FaceColorId : int { F0 = 0, F1 = 1, F2 = 2, F3 = 3 };

// Body vertex index (rotation axis passes through origin and this vertex).
enum class BodyVertex : int { V0 = 0, V1 = 1, V2 = 2, V3 = 3 };

class Tetrahedron {
public:
    static constexpr int kFaces = 4;
    static constexpr int kStickersPerFace = 16;
    static constexpr int kSubdivision = 4;

    Tetrahedron();

    // Reset stickers so each macro face shows a single solved color.
    void reset();

    // Rotate cap around BodyVertex v; depth in {1,2,3} selects rows from that vertex (1 = tip only).
    // ccw: when looking from outside along +axis (origin -> vertex), true = counter-clockwise 120 degrees.
    void rotate(int vertex, int depth, bool ccw);

    bool applyMoveToken(const std::string& token);

    void scramble(int numMoves = 45);

    bool isSolved() const;

    const std::array<std::array<int, kStickersPerFace>, kFaces>& getFaces() const { return faces_; }

    // Geometry accessors (world space, tetrahedron centered near origin).
    static sf::Vector3f bodyVertexPosition(int bodyVertex);
    static const std::array<std::array<int, 3>, kFaces>& faceCornerVertices();

    sf::Vector3f stickerCenter(int face, int slot) const;
    bool stickerInCap(int face, int slot, int vertex, int depth) const;

    // True if sticker normal is "up" toward first face corner (tetractys upward triangles in row layout).
    static bool stickerIsUpTriangle(int slot);

private:
    std::array<std::array<int, kStickersPerFace>, kFaces> faces_{};

    struct Geometry {
        std::array<std::array<sf::Vector3f, kStickersPerFace>, kFaces> centers{};
        std::array<std::array<std::array<float, 3>, kStickersPerFace>, kFaces> bary{};
        bool built = false;
    };

    mutable Geometry geom_{};

    void ensureGeometry() const;
    static int flatIndex(int face, int slot);
    static void unflatIndex(int flat, int& face, int& slot);

    int findNearestSticker(const sf::Vector3f& world) const;
    sf::Vector3f rotateAroundAxis(const sf::Vector3f& p, const sf::Vector3f& axisUnit, float radians) const;
};

#endif
