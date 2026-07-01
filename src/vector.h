#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

namespace vectorFunctions{
    struct Vector3D{
        float x, y, z;
    };

    struct SpotLight{
        Vector3D position;
        Vector3D direction; // Normalized //
        float power {100}; // 100 is default lighting //
        float cutOff;
        float XZRotation;
        float YZRotation;
    };

    struct PointLight{
        Vector3D position;
        float power {100}; // 100 is default lighting //
    };

    float dotProduct(const Vector3D& v0, const Vector3D& v1);
    Vector3D crossProduct(const Vector3D& v0, const Vector3D& v1);
    Vector3D add3D(const Vector3D& v0, const Vector3D& v1);
    vectorFunctions::Vector3D subtract3D(const vectorFunctions::Vector3D& v0, const vectorFunctions::Vector3D& v1);
    Vector3D multiply3D(const Vector3D& v, const float factor);
    float length(const Vector3D& v);
    float reciprocalLength(const Vector3D& v);
    Vector3D normalize(const Vector3D& v);
    Vector3D fastNormalize(const Vector3D& v);
    Vector3D convertAngle(float angleYaw, float anglePitch);

    // Lights //
    extern std::vector<Vector3D> directionalLights;
    extern std::vector<SpotLight> spotLights;
    extern std::vector<PointLight> pointLights;
}

#endif
