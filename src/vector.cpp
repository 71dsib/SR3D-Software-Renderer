#include "call.h"

namespace vectorFunctions{
    float dotProduct(const Vector3D& v0, const Vector3D& v1){
        return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
    }
    Vector3D crossProduct(const Vector3D& v0, const Vector3D& v1){
        return {v0.y * v1.z - v0.z * v1.y, v0.z * v1.x - v0.x * v1.z, v0.x * v1.y - v0.y * v1.x};
    }
    Vector3D add3D(const Vector3D& v0, const Vector3D& v1){
        return {v0.x + v1.x, v0.y + v1.y, v0.z + v1.z};
    }
    Vector3D divide3D(const Vector3D& v, const float divisor){
        return {v.x / divisor, v.y / divisor, v.z / divisor};
    }
    Vector3D multiply3D(const Vector3D& v, const float factor){
        return {v.x * factor, v.y * factor, v.z * factor};
    }
    float length(const Vector3D& v){
        return std::sqrt(dotProduct(v,v));
    }
    float reciprocalLength(const Vector3D& v){
        return 1.0f/std::sqrt(dotProduct(v,v));
    }
    Vector3D normalize(const Vector3D& v) {
        float vLength = length(v);

        if (vLength == 0.0f){
            return {0, 0, 0};
        }

        return divide3D(v, vLength);
    }
    Vector3D fastNormalize(const Vector3D& v){
        float vRLength = reciprocalLength(v);
        return multiply3D(v, vRLength);
    }
    Vector3D convertAngle(float angleYaw, float anglePitch){
        float radianYaw {angleYaw * render::radian};
        float radianPitch {anglePitch * render::radian};
        return {(-std::sin(radianYaw) * std::cos(radianPitch)),std::sin(radianPitch),(-std::cos(radianYaw) * std::cos(radianPitch))};
    }
    std::vector<Vector3D> directionalLights {};
    std::vector<SpotLight> spotLights {{{8 , 5 , 0}, convertAngle(275,25), 100,std::cos(25*render::radian), 275,25}};
    std::vector<PointLight> pointLights {};

}

 namespace shadowFunctions{
    std::vector<ShadowMap> shadowMaps {};
    render::Point3D toSpotLightSpace(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight) {
        float lx {spotLight.position.x};
        float ly {spotLight.position.y};
        float lz {spotLight.position.z};
        float x {point.x - lx};
        float y {point.y - ly};
        float z {point.z - lz};
        float lightSinXZ {std::sin(spotLight.XZRotation * render::radian)};
        float lightCosXZ {std::cos(spotLight.XZRotation * render::radian)};
        float tempX {x};
        x = (x * lightCosXZ) - (z * lightSinXZ);
        z = (tempX * lightSinXZ) + (z * lightCosXZ);
        float lightSinYZ {std::sin(spotLight.YZRotation * render::radian)};
        float lightCosYZ {std::cos(spotLight.YZRotation * render::radian)};
        float tempY {y};
        y = (y * lightCosYZ) - (z * lightSinYZ);
        z = (tempY * lightSinYZ) + (z * lightCosYZ);
        return {x, y, z, point.u, point.v};
    }
    render::Point3D fromSLToWorldSpace(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight) {
        float x {point.x};
        float y {point.y};
        float z {point.z};
        float lx {spotLight.position.x};
        float ly {spotLight.position.y};
        float lz {spotLight.position.z};
        float reverseSinXZ {std::sin(-spotLight.XZRotation * render::radian)};
        float reverseCosXZ {std::cos(-spotLight.XZRotation * render::radian)};
        float reverseSinYZ {std::sin(-spotLight.YZRotation * render::radian)};
        float reverseCosYZ {std::cos(-spotLight.YZRotation * render::radian)};
        float tempY {y};
        y = (y * reverseCosYZ) - (z * reverseSinYZ);
        z = (tempY * reverseSinYZ) + (z * reverseCosYZ);
        float tempX {x};
        x = (x * reverseCosXZ) - (z * reverseSinXZ);
        z = (tempX * reverseSinXZ) + (z * reverseCosXZ);
        return {x + lx, y + ly, z + lz, point.u, point.v};
    }
    render::Point2D projectPointSL(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight, int width, int height){
        float x {point.x};
        float y {point.y};
        float z {point.z};
        float lx {spotLight.position.x};
        float ly {spotLight.position.y};
        float lz {spotLight.position.z};
        float projectedX {};
        float projectedY {};
        bool pointShown {true};
        {
            float lightSinXZ {std::sin(spotLight.XZRotation * render::radian)};
            float lightCosXZ {std::cos(spotLight.XZRotation * render::radian)};
            float tempX {x};
            x = ((x - lx) * lightCosXZ) - ((z - lz) * lightSinXZ) + lx;
            z = ((tempX - lx) * lightSinXZ) + ((z - lz) * lightCosXZ) + lz;
        }
        {
            float lightSinYZ {std::sin(spotLight.YZRotation * render::radian)};
            float lightCosYZ {std::cos(spotLight.YZRotation * render::radian)};
            float tempY {y};
            y = ((y - ly) * lightCosYZ) - ((z - lz) * lightSinYZ) + ly;
            z = ((tempY - ly) * lightSinYZ) + ((z - lz) * lightCosYZ) + lz;
        }
        float camZ {z - lz};
        if(camZ <= render::nearPlane){
            pointShown = false;
        }
        {
            float lineA {y-ly};
            float lineB {z - lz};
            float lineT {static_cast<float>(render::screenDistance)};
            float lineQ {(lineA*lineT)/lineB};
            projectedY = lineQ;
        }
        {
            float lineA {x-lx};
            float lineB {z - lz};
            float lineT {static_cast<float>(render::screenDistance)};
            float lineQ {(lineA*lineT)/lineB};
            projectedX = lineQ;
        }
        render::Point2D projectedPoint = coordinates::convertCoordinates({projectedX, projectedY, z, pointShown}, width, height);
        return projectedPoint;
    }
    void rasterizeTriangleSL(render::model::Triangle tri, const vectorFunctions::SpotLight& spotLight, ShadowMap& shadowMap){
        // Pre-Rasterization //
        render::Point2D p0 {projectPointSL(tri.p0, spotLight, shadowMap.width, shadowMap.height)};
        render::Point2D p1 {projectPointSL(tri.p1, spotLight, shadowMap.width, shadowMap.height)};
        render::Point2D p2 {projectPointSL(tri.p2, spotLight, shadowMap.width, shadowMap.height)};
        bool clockwise {(render::edgeFunction(p0,p1,p2) > 0) ? true : false};
        int maxX {static_cast<int>(std::round(std::max({p0.x, p1.x, p2.x})))};
        int maxY {static_cast<int>(std::round(std::max({p0.y, p1.y, p2.y})))};
        int minX {static_cast<int>(std::round(std::min({p0.x, p1.x, p2.x})))};
        int minY {static_cast<int>(std::round(std::min({p0.y, p1.y, p2.y})))};
        minX = std::max(0, minX);
        minY = std::max(0, minY);
        maxX = std::min(shadowMap.width - 1, maxX);
        maxY = std::min(shadowMap.height - 1, maxY);
        float area {render::edgeFunction(p0,p1,p2)};
        float cZ0 {p0.sZ - spotLight.position.z};
        float cZ1 {p1.sZ - spotLight.position.z};
        float cZ2 {p2.sZ - spotLight.position.z};
        if (std::abs(area) < 1e-6f){
            return;
        }
        // Rasterization //
        for(int y {minY}; y <= maxY; y++){
            for(int x {minX}; x <= maxX; x++){
                render::Point2D p {x,y};
                float e0 {render::edgeFunction(p,p1,p2)}; // Edge Point 1 & Point 2 //
                float e1 {render::edgeFunction(p,p2,p0)}; // Edge Point 0 & Point 2 //
                float e2 {render::edgeFunction(p,p0,p1)}; // Edge Point 0 & Point 1 //
                if((e0 >= 0 && e1 >= 0 && e2 >= 0 && clockwise)|| (e0 <= 0 && e1 <= 0 && e2 <= 0 && !clockwise)){
                    float w0 {e0/area};
                    float w1 {e1/area};
                    float w2 {e2/area};
                    float iZ {((w0/cZ0)+(w1/cZ1)+(w2/cZ2))};
                    float depth {1.0f/iZ};
                    if(depth < shadowMap.buffer[y*shadowMap.width+x]){
                        shadowMap.buffer[y*shadowMap.width+x] = depth;
                    }
                }
            }
        }
    }
    void rasterizeTriangleSLClip(render::model::Triangle tri, const vectorFunctions::SpotLight& spotLight, ShadowMap& shadowMap){
        render::model::Triangle lightTri {tri};
        lightTri.p0 = toSpotLightSpace(tri.p0, spotLight);
        lightTri.p1 = toSpotLightSpace(tri.p1, spotLight);
        lightTri.p2 = toSpotLightSpace(tri.p2, spotLight);
        std::vector<render::model::Triangle> triangles {clip::clipTriangle(lightTri,false)};
        for(int i {0}; i < triangles.size(); i++){
            triangles[i].p0 = fromSLToWorldSpace(triangles[i].p0, spotLight);
            triangles[i].p1 = fromSLToWorldSpace(triangles[i].p1, spotLight);
            triangles[i].p2 = fromSLToWorldSpace(triangles[i].p2, spotLight);
            rasterizeTriangleSL(triangles[i], spotLight, shadowMap);
        }
    }
}
