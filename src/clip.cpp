#include "call.h"

namespace clip{
    render::Point3D toCameraSpace(const render::Point3D& point) {
        float x {point.x - render::cameraX};
        float y {point.y - render::cameraY};
        float z {point.z - render::cameraZ};

        float tempX {x};
        x = (x * render::cameraRotationCosXZ) - (z * render::cameraRotationSinXZ);
        z = (tempX * render::cameraRotationSinXZ) + (z * render::cameraRotationCosXZ);

        float tempY {y};
        y = (y * render::cameraRotationCosYZ) - (z * render::cameraRotationSinYZ);
        z = (tempY * render::cameraRotationSinYZ) + (z * render::cameraRotationCosYZ);

        return {x, y, z, point.u, point.v};
    }

    render::Point3D toWorldSpace(const render::Point3D& point) {
        float x {point.x};
        float y {point.y};
        float z {point.z};

        float reverseSinXZ {std::sin(-render::cameraRotationXZ * render::radian)};
        float reverseCosXZ {std::cos(-render::cameraRotationXZ * render::radian)};

        float reverseSinYZ {std::sin(-render::cameraRotationYZ * render::radian)};
        float reverseCosYZ {std::cos(-render::cameraRotationYZ * render::radian)};

        float tempY {y};
        y = (y * reverseCosYZ) - (z * reverseSinYZ);
        z = (tempY * reverseSinYZ) + (z * reverseCosYZ);

        float tempX {x};
        x = (x * reverseCosXZ) - (z * reverseSinXZ);
        z = (tempX * reverseSinXZ) + (z * reverseCosXZ);

        return {x + render::cameraX, y + render::cameraY, z + render::cameraZ, point.u, point.v};
    }

    render::Point3D returnPlaneIntercept(const render::Point3D& pointA, const render::Point3D& pointB, float plane){
        render::Point3D returnPoint {};
        float t {(plane-pointA.z)/(pointB.z-pointA.z)};

        returnPoint.x = t*(pointB.x - pointA.x) + pointA.x;
        returnPoint.y = t*(pointB.y - pointA.y) + pointA.y;
        returnPoint.z = t*(pointB.z - pointA.z) + pointA.z;

        returnPoint.u = t*(pointB.u - pointA.u) + pointA.u;
        returnPoint.v = t*(pointB.v - pointA.v) + pointA.v;

        return returnPoint;
    }

    float returnT(const render::Point3D& pointA, const render::Point3D& pointB, float plane){
        float t {(plane-pointA.z)/(pointB.z-pointA.z)};
        return t;
    }

    vectorFunctions::Vector3D returnPlaneInterceptN(const vectorFunctions::Vector3D& vectorA, const vectorFunctions::Vector3D& vectorB, float plane, float t){
        vectorFunctions::Vector3D returnVector {};

        returnVector.x = t*(vectorB.x - vectorA.x) + vectorA.x;
        returnVector.y = t*(vectorB.y - vectorA.y) + vectorA.y;
        returnVector.z = t*(vectorB.z - vectorA.z) + vectorA.z;

        return returnVector;
    }
    void makeTriangleClockwise(render::model::Triangle& tri, bool inCamSpace){
        render::Point2D p0 {};
        render::Point2D p1 {};
        render::Point2D p2 {};

        if(inCamSpace){
            p0 = render::projectPoint(toWorldSpace(tri.p0));
            p1 = render::projectPoint(toWorldSpace(tri.p1));
            p2 = render::projectPoint(toWorldSpace(tri.p2));
        }else{
            p0 = render::projectPoint(tri.p0);
            p1 = render::projectPoint(tri.p1);
            p2 = render::projectPoint(tri.p2);
        }

        bool clockwise {(render::edgeFunction(p0,p1,p2) > 0) ? true : false};

        if(!clockwise){
            std::swap(tri.p1,tri.p2);
            std::swap(tri.n1, tri.n2);
        }
    }


    std::vector<render::model::Triangle> clipTriangle(const render::model::Triangle& tri, bool makeClockwise){
        render::Point3D points[3] {tri.p0, tri.p1, tri.p2};
        vectorFunctions::Vector3D normals[3] {{0,0,0}, {0,0,0}, {0,0,0}};
        if(tri.hasNormals){
            normals[0] = tri.n0;
            normals[1] = tri.n1;
            normals[2] = tri.n2;
        }

        std::vector<render::model::Triangle> returnTriangles {};
        std::vector<int> onScreenPoints {};
        std::vector<int> offScreenPoints {};
        for(int i {0}; i < 3; i++){
            if(points[i].z < render::nearPlane){
                offScreenPoints.push_back(i);
            }else{
                onScreenPoints.push_back(i);
            }
        }
        switch(offScreenPoints.size()){
            case 0:{
                returnTriangles.push_back(tri);
                break;
            }
            case 1:{
                render::model::Triangle newTriangleA {};
                render::model::Triangle newTriangleB {};

                // Triangle A //
                newTriangleA.p0 = points[onScreenPoints[0]];
                newTriangleA.p1 = points[onScreenPoints[1]];
                newTriangleA.p2 = returnPlaneIntercept(points[onScreenPoints[0]], points[offScreenPoints[0]], render::nearPlane);

                newTriangleA.n0 = normals[onScreenPoints[0]];

                float t1A {returnT(points[onScreenPoints[0]], points[offScreenPoints[0]], render::nearPlane)};
                newTriangleA.n1 = returnPlaneInterceptN(normals[onScreenPoints[0]], normals[offScreenPoints[0]], render::nearPlane, t1A);

                float t2A {returnT(points[onScreenPoints[1]], points[offScreenPoints[0]], render::nearPlane)};
                newTriangleA.n2 = returnPlaneInterceptN(normals[onScreenPoints[1]], normals[offScreenPoints[0]], render::nearPlane, t2A);

                // Triangle B //
                newTriangleB.p0 = points[onScreenPoints[1]];
                newTriangleB.p1 = newTriangleA.p2;
                newTriangleB.p2 = returnPlaneIntercept(points[onScreenPoints[1]], points[offScreenPoints[0]], render::nearPlane);

                newTriangleB.n0 = normals[onScreenPoints[1]];

                float t1B {returnT(points[onScreenPoints[1]], points[offScreenPoints[0]], render::nearPlane)};
                newTriangleB.n1 = returnPlaneInterceptN(normals[onScreenPoints[1]], normals[offScreenPoints[0]], render::nearPlane, t1B);

                float t2B {returnT(points[onScreenPoints[0]], points[offScreenPoints[0]], render::nearPlane)};
                newTriangleB.n2 = returnPlaneInterceptN(normals[onScreenPoints[0]], normals[offScreenPoints[0]], render::nearPlane, t2B);

                newTriangleA.texture = tri.texture;
                newTriangleB.texture = tri.texture;

                if(makeClockwise){
                    makeTriangleClockwise(newTriangleA, true);
                    makeTriangleClockwise(newTriangleB, true);
                }

                returnTriangles.push_back(newTriangleA);
                returnTriangles.push_back(newTriangleB);
                break;
            }
            case 2:{
                render::model::Triangle newTriangle {};
                newTriangle.p0 = points[onScreenPoints[0]];
                newTriangle.p1 = returnPlaneIntercept(points[onScreenPoints[0]], points[offScreenPoints[0]], render::nearPlane);
                newTriangle.p2 = returnPlaneIntercept(points[onScreenPoints[0]], points[offScreenPoints[1]], render::nearPlane);

                newTriangle.n0 = normals[onScreenPoints[0]];

                float t1 {returnT(points[onScreenPoints[0]], points[offScreenPoints[0]], render::nearPlane)};
                newTriangle.n1 = returnPlaneInterceptN(normals[onScreenPoints[0]], normals[offScreenPoints[0]], render::nearPlane, t1);

                float t2 {returnT(points[onScreenPoints[0]], points[offScreenPoints[1]], render::nearPlane)};
                newTriangle.n2 = returnPlaneInterceptN(normals[onScreenPoints[0]], normals[offScreenPoints[1]], render::nearPlane, t2);

                newTriangle.texture = tri.texture;

                if(makeClockwise){
                    makeTriangleClockwise(newTriangle, true);
                }

                returnTriangles.push_back(newTriangle);
                break;
            }
        }
        return returnTriangles;
    }
}
