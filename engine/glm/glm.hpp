#pragma once

#include <cmath>
#include <cstddef>

namespace glm {

struct vec3 {
    float x;
    float y;
    float z;

    vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    explicit vec3(float v) : x(v), y(v), z(v) {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float& operator[](std::size_t index) {
        return *(&x + index);
    }

    const float& operator[](std::size_t index) const {
        return *(&x + index);
    }
};

struct vec4 {
    float x;
    float y;
    float z;
    float w;

    vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    explicit vec4(float v) : x(v), y(v), z(v), w(v) {}
    vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    vec4(const vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    float& operator[](std::size_t index) {
        return *(&x + index);
    }

    const float& operator[](std::size_t index) const {
        return *(&x + index);
    }
};

struct mat4 {
    float data[4][4];

    mat4() {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                data[c][r] = (c == r) ? 1.0f : 0.0f;
            }
        }
    }

    explicit mat4(float diagonal) {
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                data[c][r] = (c == r) ? diagonal : 0.0f;
            }
        }
    }

    float* operator[](std::size_t column) {
        return data[column];
    }

    const float* operator[](std::size_t column) const {
        return data[column];
    }
};

inline vec3 operator+(const vec3& a, const vec3& b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline vec3 operator-(const vec3& a, const vec3& b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline vec3 operator*(const vec3& v, float s) {
    return vec3(v.x * s, v.y * s, v.z * s);
}

inline vec3 operator*(float s, const vec3& v) {
    return v * s;
}

inline vec3 operator/(const vec3& v, float s) {
    return vec3(v.x / s, v.y / s, v.z / s);
}

inline vec4 operator*(const vec4& v, float s) {
    return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline float radians(float degrees) {
    return degrees * static_cast<float>(M_PI / 180.0);
}

inline float dot(const vec3& a, const vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline float length(const vec3& v) {
    return std::sqrt(dot(v, v));
}

inline vec3 normalize(const vec3& v) {
    float len = length(v);
    if (len > 0.0f) {
        return v / len;
    }
    return vec3(0.0f);
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 result(1.0f);
    result[0][0] = s.x;
    result[0][1] = u.x;
    result[0][2] = -f.x;
    result[0][3] = 0.0f;

    result[1][0] = s.y;
    result[1][1] = u.y;
    result[1][2] = -f.y;
    result[1][3] = 0.0f;

    result[2][0] = s.z;
    result[2][1] = u.z;
    result[2][2] = -f.z;
    result[2][3] = 0.0f;

    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    result[3][3] = 1.0f;

    return result;
}

inline mat4 perspective(float fovyRadians, float aspect, float zNear, float zFar) {
    const float tanHalfFovy = std::tan(fovyRadians / 2.0f);

    mat4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tanHalfFovy);
    result[1][1] = 1.0f / tanHalfFovy;
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0f;
    result[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return result;
}

inline const float* value_ptr(const mat4& m) {
    return &m.data[0][0];
}

inline float* value_ptr(mat4& m) {
    return &m.data[0][0];
}

inline const float* value_ptr(const vec3& v) {
    return &v.x;
}

inline const float* value_ptr(const vec4& v) {
    return &v.x;
}

} // namespace glm

