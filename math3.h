// Small 3D math helpers for mesh generation and rigid transforms (no external deps).

#pragma once

#include <cmath>

/** Homogeneous 3-vector for positions, directions, and barycentric weights. */
struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Vec3() = default;
    Vec3(float ax, float ay, float az) : x(ax), y(ay), z(az) {}

    /** Component-wise sum. */
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    /** Component-wise difference. */
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    /** Uniform scale. */
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    /** Uniform divide (caller avoids zero). */
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
};

/** Dot product (length² when a==b). */
inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(Vec3 v) {
    return std::sqrt(dot(v, v));
}

/** Unit vector; returns zero vector if length is negligible. */
inline Vec3 normalize(Vec3 v) {
    const float len = length(v);
    if (len < 1e-20f) {
        return {0.f, 0.f, 0.f};
    }
    return v * (1.f / len);
}

/** Rodrigues: rotate vector v around unit axis by angle radians (right-hand rule). */
inline Vec3 rotateAroundAxis(Vec3 v, Vec3 axisUnit, float angleRad) {
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);
    const Vec3 k = axisUnit;
    const float t = 1.f - c;
    return {v.x * (t * k.x * k.x + c) + v.y * (t * k.x * k.y - s * k.z) +
                v.z * (t * k.x * k.z + s * k.y),
            v.x * (t * k.x * k.y + s * k.z) + v.y * (t * k.y * k.y + c) +
                v.z * (t * k.y * k.z - s * k.x),
            v.x * (t * k.x * k.z - s * k.y) + v.y * (t * k.y * k.z + s * k.x) +
                v.z * (t * k.z * k.z + c)};
}

/** Rotate point p around axis through pivot; axis direction is unit. */
inline Vec3 rotatePointAroundAxis(Vec3 p, Vec3 pivot, Vec3 axisUnit, float angleRad) {
    return pivot + rotateAroundAxis(p - pivot, axisUnit, angleRad);
}

/**
 * Barycentric coordinates of p on triangle (a,b,c); u is weight on a, v on b, w on c (u+v+w=1).
 * Robust for coplanar p; falls back if degenerate.
 */
inline void barycentric3(Vec3 p, Vec3 a, Vec3 b, Vec3 c, float& u, float& v, float& w) {
    const Vec3 v0 = b - a;
    const Vec3 v1 = c - a;
    const Vec3 v2 = p - a;
    const float d00 = dot(v0, v0);
    const float d01 = dot(v0, v1);
    const float d11 = dot(v1, v1);
    const float d20 = dot(v2, v0);
    const float d21 = dot(v2, v1);
    const float denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-24f) {
        u = 1.f;
        v = w = 0.f;
        return;
    }
    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.f - v - w;
}
