#include "call.h"

namespace render{
    void rasterizeTriangle(model::Triangle tri, float tileFactor){
        // Pre-Rasterization //
        Point2D p0 {projectPoint(tri.p0)};
        Point2D p1 {projectPoint(tri.p1)};
        Point2D p2 {projectPoint(tri.p2)};


        bool clockwise {(edgeFunction(p0,p1,p2) > 0) ? true : false};
        if(!clockwise){
            return;
        }

        int maxX {static_cast<int>(std::round(std::max({p0.x, p1.x, p2.x})))};
        int maxY {static_cast<int>(std::round(std::max({p0.y, p1.y, p2.y})))};


        int minX {static_cast<int>(std::round(std::min({p0.x, p1.x, p2.x})))};
        int minY {static_cast<int>(std::round(std::min({p0.y, p1.y, p2.y})))};

        minX = std::max(0, minX);
        minY = std::max(0, minY);

        maxX = std::min(screenW - 1, maxX);
        maxY = std::min(screenH - 1, maxY);

        float area {edgeFunction(p0,p1,p2)};

        float cZ0 {p0.sZ - cameraZ};
        float cZ1 {p1.sZ - cameraZ};
        float cZ2 {p2.sZ - cameraZ};

        // Rasterization //
        for(int y = minY; y <= maxY; y++){
            for(int x = minX; x <= maxX; x++){
                Point2D p {x,y};

                float e0 {edgeFunction(p,p1,p2)}; // Edge Point 1 & Point 2 //
                float e1 {edgeFunction(p,p2,p0)}; // Edge Point 0 & Point 2 //
                float e2 {edgeFunction(p,p0,p1)}; // Edge Point 0 & Point 1 //

                if((e0 >= 0 && e1 >= 0 && e2 >= 0 && clockwise) || (e0 <= 0 && e1 <= 0 && e2 <= 0 && !clockwise)){
                    float w0 {e0/area};
                    float w1 {e1/area};
                    float w2 {e2/area};

                    float iZ {((w0/cZ0)+(w1/cZ1)+(w2/cZ2))};
                    float depth {1.0f/iZ};

                    if(depth >= zBuffer[y*screenW+x] + 0.001f){
                        continue;
                    }

                    vectorFunctions::Vector3D normal {vectorFunctions::add3D(vectorFunctions::add3D(vectorFunctions::multiply3D(tri.n0, w0), vectorFunctions::multiply3D(tri.n1, w1)), vectorFunctions::multiply3D(tri.n2, w2))}; // This doesn't look like it but this is interpolating //
                    normal = vectorFunctions::fastNormalize(normal);

                    vectorFunctions::Vector3D pixel3DPos {}; // I used Vector3D instead of Point3D because this is used in Vector Operations so it's easier to leave it like this //

                    float pixel3DPosX {(((tri.p0.x * w0)/cZ0) + ((tri.p1.x * w1)/cZ1) + ((tri.p2.x * w2)/cZ2))/iZ};
                    float pixel3DPosY {(((tri.p0.y * w0)/cZ0) + ((tri.p1.y * w1)/cZ1) + ((tri.p2.y * w2)/cZ2))/iZ};
                    float pixel3DPosZ {(((tri.p0.z * w0)/cZ0) + ((tri.p1.z * w1)/cZ1) + ((tri.p2.z * w2)/cZ2))/iZ};
                    pixel3DPos = {pixel3DPosX, pixel3DPosY, pixel3DPosZ};

                    float intensity {computeLighting(normal, pixel3DPos)};

                    float u {(((tri.p0.u * w0)/cZ0) + ((tri.p1.u * w1)/cZ1) + ((tri.p2.u * w2)/cZ2))/iZ};
                    float v {(((tri.p0.v * w0)/cZ0) + ((tri.p1.v * w1)/cZ1) + ((tri.p2.v * w2)/cZ2))/iZ};

                    float wrappedU {(u * tileFactor) - std::floor(u * tileFactor)};
                    float wrappedV {(v * tileFactor) - std::floor(v * tileFactor)};

                    int textureX {static_cast<int>(wrappedU*(tri.texture->width))};
                    int textureY {static_cast<int>(wrappedV*(tri.texture->height))};

                    textureX = std::clamp(textureX, 0, tri.texture->width-1);
                    textureY = std::clamp(textureY, 0, tri.texture->height-1);
                    uint32_t pixelColor {tri.texture->data[textureY * tri.texture->width + textureX]};
                    colorBuffer[y*screenW+x] = applyIntensity(pixelColor, intensity);
                    zBuffer[y*screenW+x] = depth;
                }
            }
        }
    }
    void rasterizeTriangleClip(model::Triangle tri, float tileFactor){
        model::Triangle cameraTri {tri};
        cameraTri.p0 = clip::toCameraSpace(tri.p0);
        cameraTri.p1 = clip::toCameraSpace(tri.p1);
        cameraTri.p2 = clip::toCameraSpace(tri.p2);

        std::vector<model::Triangle> triangles {clip::clipTriangle(cameraTri)};

        for(int i {0}; i < triangles.size(); i++){
            triangles[i].p0 = clip::toWorldSpace(triangles[i].p0);
            triangles[i].p1 = clip::toWorldSpace(triangles[i].p1);
            triangles[i].p2 = clip::toWorldSpace(triangles[i].p2);
            rasterizeTriangle(triangles[i], tileFactor);
        }
    }
}
