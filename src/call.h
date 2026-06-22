#ifndef INITIALIZED
#define INITIALIZED

#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <random>
#include <iomanip>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <execution>
#include <cfloat>
#include <omp.h>

namespace antiAlias{
    void runFXAA(uint32_t* colorBuffer, uint32_t* finalBuffer, int width, int height);
}


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

namespace shadowFunctions{
    struct ShadowMap{
        int width {};
        int height {};
        std::vector<float> buffer;

        void allocate(){
            buffer.resize(width * height, FLT_MAX);
        }
    };

    extern std::vector<ShadowMap> shadowMaps;
}

namespace coordinates{
    extern long int triangleCount;
}

namespace render{
    struct Point2D;
    struct Point3D;

    namespace model{
        struct Texture;
        struct Triangle;
        struct ModelTriangle;
        struct Model;
    }
}

namespace load{
    extern std::vector<render::model::Texture> loadedTextures;
    extern std::vector<render::model::Model> loadedModels;
}

namespace render{
    // Point Data Types //
    struct Point2D{
        float x, y, sZ;
        bool shown {true};
    };
    struct Point3D{
        float x, y, z, u, v;
    };

    vectorFunctions::Vector3D subtract3D(const render::Point3D& v0, const render::Point3D& v1);// I could not put this in vector functions (it needed Point3D) so I'm temporarily leaving this here //
    vectorFunctions::Vector3D subtract3D(const vectorFunctions::Vector3D& v0, const vectorFunctions::Vector3D& v1);

    // Render Variables //
    const unsigned short int screenW {1920};
    const unsigned short int screenH {1080};

    extern uint32_t colorBuffer[screenW * screenH];
    extern uint32_t finalBuffer[screenW * screenH];
    extern float zBuffer[screenW * screenH];

    // Camera Variables //

    extern float cameraX; // Camera Point //
    extern float cameraY;
    extern float cameraZ;

    extern float nearPlane;

    extern float radian;

    extern int screenDistance; // Screen Distance //
    extern float cameraRotationXZ;
    extern float cameraRotationXY;
    extern float cameraRotationYZ;

    extern float cameraRotationSinXZ;
    extern float cameraRotationCosXZ;

    extern float cameraRotationSinXY;
    extern float cameraRotationCosXY;

    extern float cameraRotationSinYZ;
    extern float cameraRotationCosYZ;

    render::Point3D toCameraSpaceZ(const render::Point3D& point);

    namespace model{
        struct Texture{
            int width {};
            int height {};
            int tileFactor {1};
            std::vector<uint32_t> data;
        };
        struct Triangle{
            render::Point3D p0, p1, p2;
            vectorFunctions::Vector3D n0, n1, n2;
            bool hasNormals {false};
            Texture* texture {};
        };
        struct Triangle2D{
            render::Point2D p0, p1, p2;
            vectorFunctions::Vector3D n0, n1, n2;
            bool hasNormals {false};
            Texture* texture {};
        };
        struct ModelTriangle{
            int pIndex0, pIndex1, pIndex2;
            int nIndex0, nIndex1, nIndex2;
            float u0, v0, u1, v1, u2, v2;
            bool hasNormals {false};
            float approxDepth {};
            Texture* texture {};
            render::model::Triangle convert(const std::vector<render::Point3D>& points, const std::vector<vectorFunctions::Vector3D>& normals){
                render::Point3D p0 {points[pIndex0].x, points[pIndex0].y, points[pIndex0].z, u0, v0};
                render::Point3D p1 {points[pIndex1].x, points[pIndex1].y, points[pIndex1].z, u1, v1};
                render::Point3D p2 {points[pIndex2].x, points[pIndex2].y, points[pIndex2].z, u2, v2};
                render::model::Triangle returnTriangle {p0, p1, p2, normals[nIndex0], normals[nIndex1], normals[nIndex2], hasNormals, texture};
                return returnTriangle;
            }
            render::model::Triangle convert(const std::vector<render::Point3D>& points){
                vectorFunctions::Vector3D tempNormal {0,0,0};
                render::Point3D p0 {points[pIndex0].x, points[pIndex0].y, points[pIndex0].z, u0, v0};
                render::Point3D p1 {points[pIndex1].x, points[pIndex1].y, points[pIndex1].z, u1, v1};
                render::Point3D p2 {points[pIndex2].x, points[pIndex2].y, points[pIndex2].z, u2, v2};
                render::model::Triangle returnTriangle {p0, p1, p2, tempNormal, tempNormal, tempNormal, hasNormals, texture};
                return returnTriangle;
            }
        };
        struct Model{
            std::vector<render::Point3D> points {};
            std::vector<vectorFunctions::Vector3D> normals {};
            std::vector<render::model::ModelTriangle> triangles {};
            render::Point3D center {};
            float radius {};

            void setTexture(Texture& myTexture){
                for(int i {0}; i < triangles.size(); i++){
                    triangles[i].texture = &myTexture;
                }
            }
            void computeNormalsFlat(){
                std::vector<vectorFunctions::Vector3D> normalAccumalator(points.size(), {0,0,0});

                for(int i {0}; i < triangles.size(); i++){
                    render::model::ModelTriangle& triangle {triangles[i]};

                    render::Point3D p0 {points[triangle.pIndex0]};
                    render::Point3D p1 {points[triangle.pIndex1]};
                    render::Point3D p2 {points[triangle.pIndex2]};

                    vectorFunctions::Vector3D edge1 {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
                    vectorFunctions::Vector3D edge2 {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

                    vectorFunctions::Vector3D faceNormals {vectorFunctions::crossProduct(edge1, edge2)};

                    normalAccumalator[triangle.pIndex0] = vectorFunctions::add3D(normalAccumalator[triangle.pIndex0], faceNormals);
                    normalAccumalator[triangle.pIndex1] = vectorFunctions::add3D(normalAccumalator[triangle.pIndex1], faceNormals);
                    normalAccumalator[triangle.pIndex2] = vectorFunctions::add3D(normalAccumalator[triangle.pIndex2], faceNormals);

                    triangle.hasNormals = true;
                }

                normals.clear();
                for(int i {0}; i < normalAccumalator.size(); i++){
                    normals.push_back(vectorFunctions::normalize(vectorFunctions::multiply3D(normalAccumalator[i],-1.0f)));
                }
            }
            void computeNormalsSmooth(float maxAngle = 35){
                float minDotProduct {std::cos(maxAngle * render::radian)};
                std::vector<vectorFunctions::Vector3D> faceNormals {};

                normals.clear();

                for(int i {0}; i < triangles.size(); i++){
                    render::model::ModelTriangle& triangle {triangles[i]};

                    render::Point3D p0 {points[triangle.pIndex0]};
                    render::Point3D p1 {points[triangle.pIndex1]};
                    render::Point3D p2 {points[triangle.pIndex2]};

                    vectorFunctions::Vector3D edge1 {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
                    vectorFunctions::Vector3D edge2 {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

                    vectorFunctions::Vector3D faceNormal {vectorFunctions::normalize(vectorFunctions::crossProduct(edge1, edge2))};
                    faceNormals.push_back(faceNormal);
                }

                for(int i {0}; i < triangles.size(); i++){
                    render::model::ModelTriangle& triangleI {triangles[i]};

                    vectorFunctions::Vector3D n0 {faceNormals[i]};
                    vectorFunctions::Vector3D n1 {faceNormals[i]};
                    vectorFunctions::Vector3D n2 {faceNormals[i]};

                    for(int j {0}; j < triangles.size(); j++){
                        if(i == j || vectorFunctions::dotProduct(faceNormals[i], faceNormals[j]) < minDotProduct){
                            continue;
                        }

                        render::model::ModelTriangle& triangleJ {triangles[j]};
                        if(triangleI.pIndex0 == triangleJ.pIndex0 || triangleI.pIndex0 == triangleJ.pIndex1 || triangleI.pIndex0 == triangleJ.pIndex2){
                            n0 = add3D(n0, faceNormals[j]);
                        }
                        if(triangleI.pIndex1 == triangleJ.pIndex0 || triangleI.pIndex1 == triangleJ.pIndex1 || triangleI.pIndex1 == triangleJ.pIndex2){
                            n1 = add3D(n1, faceNormals[j]);
                        }
                        if(triangleI.pIndex2 == triangleJ.pIndex0 || triangleI.pIndex2 == triangleJ.pIndex1 || triangleI.pIndex2 == triangleJ.pIndex2){
                            n2 = add3D(n2, faceNormals[j]);
                        }
                    }
                    normals.push_back(vectorFunctions::normalize(vectorFunctions::multiply3D(n0, -1.0f)));
                    triangleI.nIndex0 = normals.size() - 1;

                    normals.push_back(vectorFunctions::normalize(vectorFunctions::multiply3D(n1, -1.0f)));
                    triangleI.nIndex1 = normals.size() - 1;

                    normals.push_back(vectorFunctions::normalize(vectorFunctions::multiply3D(n2, -1.0f)));
                    triangleI.nIndex2 = normals.size() - 1;

                    triangleI.hasNormals = true;
                }
            }
            void orderTriangles(){
                for(int i {0}; i < triangles.size(); i++){
                    render::model::ModelTriangle& triangle {triangles[i]};

                    render::Point3D p0 {render::toCameraSpaceZ(points[triangle.pIndex0])};
                    render::Point3D p1 {render::toCameraSpaceZ(points[triangle.pIndex1])};
                    render::Point3D p2 {render::toCameraSpaceZ(points[triangle.pIndex2])};

                    triangle.approxDepth = (p0.z + p1.z + p2.z) / 3.0f;
                }
                std::sort(triangles.begin(), triangles.end(), [](render::model::ModelTriangle a, render::model::ModelTriangle b){
                            return a.approxDepth < b.approxDepth;
                });
            }
            void findCenter(){
                float minX {points[0].x};
                float maxX {points[0].x};

                float minY {points[0].y};
                float maxY {points[0].y};

                float minZ {points[0].z};
                float maxZ {points[0].z};

                for(int i {0}; i < points.size(); i++){
                    if(points[i].x < minX){minX = points[i].x;}
                    if(points[i].x > maxX){maxX = points[i].x;}

                    if(points[i].y < minY){minY = points[i].y;}
                    if(points[i].y > maxY){maxY = points[i].y;}

                    if(points[i].z < minZ){minZ = points[i].z;}
                    if(points[i].z > maxZ){maxZ = points[i].z;}
                }
                center.x = (maxX + minX) * 0.5f;
                center.y = (maxY + minY) * 0.5f;
                center.z = (maxZ + minZ) * 0.5f;
            }
            void calculateRadius(){
                float maxDistanceSq {0};
                for(int i {0}; i < points.size(); i++){
                    float dx {points[i].x - center.x};
                    float dy {points[i].y - center.y};
                    float dz {points[i].z - center.z};

                    float distanceSq {vectorFunctions::dotProduct({dx,dy,dz}, {dx,dy,dz})};

                    if(distanceSq > maxDistanceSq){
                        maxDistanceSq = distanceSq;
                    }
                }
                radius = std::sqrt(maxDistanceSq);
            }
            std::vector<float> rawPointData(){
                std::vector <float> rawPoints {};
                for(int i {0}; i < points.size(); i++){
                    rawPoints.push_back(points[i].x);
                    rawPoints.push_back(points[i].y);
                    rawPoints.push_back(points[i].z);
                }
                return rawPoints;
            }
            std::vector<int> rawIndexData(){
                std::vector <int> rawIndices {};
                for(int i {0}; i < triangles.size(); i++){
                    rawIndices.push_back(triangles[i].pIndex0);
                    rawIndices.push_back(triangles[i].pIndex2);
                    rawIndices.push_back(triangles[i].pIndex1);
                }
                return rawIndices;
            }
        };
    }

    // Render Functions //
    void setCameraRotation(float value, int type);
    void moveCameraInDirection(float newZ);
    Point2D projectPoint(const Point3D& point);
    void resetBuffers();
    float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& c);
    float computeLighting(vectorFunctions::Vector3D& normal, vectorFunctions::Vector3D& pixel3DPos);
    uint32_t applyIntensity(const uint32_t& color, float intensity);
    void tileRasterizeModels();
    void rasterizeTriangle(model::Triangle tri, float tileFactor);
    void rasterizeTriangleClip(model::Triangle tri, float tileFactor = 1.0f);
}

namespace shadowFunctions{
    render::Point3D toSpotLightSpace(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight);
    render::Point3D fromSLToWorldSpace(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight);
    render::Point2D projectPointSL(const render::Point3D& point, const vectorFunctions::SpotLight& spotLight, int width, int height);
    void rasterizeTriangleSLClip(render::model::Triangle tri, const vectorFunctions::SpotLight& light, ShadowMap& shadowMap);
}

namespace clip{
    render::Point3D toCameraSpace(const render::Point3D& p);
    render::Point3D toWorldSpace(const render::Point3D& p);
    render::Point3D returnPlaneIntercept(const render::Point3D& pointA, const render::Point3D& pointB, float plane);
    float returnT(const render::Point3D& pointA, const render::Point3D& pointB, float plane);
    vectorFunctions::Vector3D returnPlaneInterceptN(const vectorFunctions::Vector3D& vectorA, const vectorFunctions::Vector3D& vectorB, float plane, float t);
    void makeTriangleClockwise(render::model::Triangle& tri, bool inCamSpace);
    std::vector<render::model::Triangle> clipTriangle(const render::model::Triangle& tri, bool makeClockwise = true);
}

namespace transform{
    void rotateModelXY(render::model::Model& model, float rotate);
    void rotateModelXZ(render::model::Model& model, float rotate);
    void rotateModelYZ(render::model::Model& model, float rotate);
    void setModelPos(render::model::Model& model, render::Point3D newPos);
}

namespace coordinates{
    render::Point2D convertCoordinates(render::Point2D point, int screenW, int screenH);
}

namespace load{
    int loadTexture(std::string filePath);
    int loadOBJModel(std::string filePath, int coordinateSys, bool extractNormals);
}

#endif // INITIALIZED
