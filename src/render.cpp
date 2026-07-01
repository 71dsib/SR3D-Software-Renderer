#include "call.h"


namespace antiAlias{
    float getLuma(uint32_t color){
        unsigned int r {(color >> 16) & 0xFF};
        unsigned int g {(color >> 8) & 0xFF};
        unsigned int b {color & 0xFF};

        return ((0.299f * r) + (0.587f * g) + (0.114f * b)) / 255.0f;
    }
    void runFXAA(uint32_t colorBuffer[], uint32_t finalBuffer[],int width, int height){
        const float threshold {0.08f};
        #pragma omp parallel for
        for(int y = 0; y < height; y++){
            for(int x = 0; x < width; x++){
                float pixelLuma {getLuma(colorBuffer[y*width+x])};

                float pixelNLuma {getLuma(colorBuffer[(std::clamp(y-1, 0, height-1))*width+x])};
                float pixelSLuma {getLuma(colorBuffer[(std::clamp(y+1, 0, height-1))*width+x])};
                float pixelELuma {getLuma(colorBuffer[y*width+(std::clamp(x+1, 0, width-1))])};
                float pixelWLuma {getLuma(colorBuffer[y*width+(std::clamp(x-1, 0, width-1))])};

                float lumaMax {std::fmax(std::fmax(pixelNLuma, pixelSLuma), std::fmax(std::fmax(pixelWLuma, pixelELuma), pixelLuma))};
                float lumaMin {std::fmin(std::fmin(pixelNLuma, pixelSLuma), std::fmin(std::fmin(pixelWLuma, pixelELuma), pixelLuma))};

                float contrast {lumaMax - lumaMin};

                if(contrast < threshold){
                    finalBuffer[y*width+x] = colorBuffer[y*width+x];
                    continue;
                }

                if (pixelLuma < 0.001f) {
                    finalBuffer[y * width + x] = colorBuffer[y * width + x];
                    continue;
                }


                float pixelNELuma {getLuma(colorBuffer[(std::clamp(y-1, 0, height-1))*width+(std::clamp(x+1, 0, width-1))])};
                float pixelNWLuma {getLuma(colorBuffer[(std::clamp(y-1, 0, height-1))*width+(std::clamp(x-1, 0, width-1))])};
                float pixelSELuma {getLuma(colorBuffer[(std::clamp(y+1, 0, height-1))*width+(std::clamp(x+1, 0, width-1))])};
                float pixelSWLuma {getLuma(colorBuffer[(std::clamp(y+1, 0, height-1))*width+(std::clamp(x-1, 0, width-1))])};

                float dirX {-((pixelNWLuma + pixelNELuma) - (pixelSWLuma + pixelSELuma))};
                float dirY {((pixelNWLuma + pixelSWLuma) - (pixelNELuma + pixelSELuma))};

                float dirReduce {std::max((pixelNWLuma + pixelNELuma + pixelSWLuma + pixelSELuma) * (1.0f / 16.0f), 1.0f / 256.0f)}; // Lowering 16.0f make the algorithm blur it more and increasing 256.0f makes it act harsher on subtle edges, I say just leave it as is//

                float rcpDirMin {1.0f / (std::min(std::abs(dirX), std::abs(dirY)) + dirReduce)};
                dirX = std::clamp(dirX * rcpDirMin, -12.0f, 12.0f); // Changing this [12.0f and -12.0f] increases the sampling size, I'd suggest you not changing this because it might lead to blurriness//
                dirY = std::clamp(dirY * rcpDirMin, -12.0f, 12.0f);

                // This looks complex but in reality it's just me splitting up code into individual variables to make it more readable, this is just indexing different colors //

                float offset1X {dirX * (1.0f/3.0f - 0.5f)};
                float offset1Y {dirY * (1.0f/3.0f - 0.5f)};
                int sample1X {std::clamp(x + static_cast<int>(std::round(offset1X)), 0, width - 1)};
                int sample1Y {std::clamp(y + static_cast<int>(std::round(offset1Y)), 0, height - 1)};
                unsigned int color1 {colorBuffer[sample1Y * width + sample1X]};

                float offset2X {dirX * (2.0f/3.0f - 0.5f)};
                float offset2Y {dirY * (2.0f/3.0f - 0.5f)};
                int sample2X {std::clamp(x + static_cast<int>(std::round(offset2X)), 0, width - 1)};
                int sample2Y {std::clamp(y + static_cast<int>(std::round(offset2Y)), 0, height - 1)};
                unsigned int color2 {colorBuffer[sample2Y * width + sample2X]};

                // RGB Color A (It looks complex because I stored it as uint32_t which forces me to index color channels like this) //

                unsigned int rgbA_R {(((color1 >> 16) & 0xFF) + ((color2 >> 16) & 0xFF)) / 2};
                unsigned int rgbA_G {(((color1 >> 8) & 0xFF) + ((color2 >> 8) & 0xFF)) / 2};
                unsigned int rgbA_B {((color1 & 0xFF) + (color2 & 0xFF)) / 2};




                float offset3X {dirX * -0.5f};
                float offset3Y {dirY * -0.5f};
                int sample3X {std::clamp(x + static_cast<int>(std::round(offset3X)), 0, width - 1)};
                int sample3Y {std::clamp(y + static_cast<int>(std::round(offset3Y)), 0, height - 1)};
                unsigned int color3 {colorBuffer[sample3Y * width + sample3X]};

                float offset4X {dirX * 0.5f};
                float offset4Y {dirY * 0.5f};
                int sample4X {std::clamp(x + static_cast<int>(std::round(offset4X)), 0, width - 1)};
                int sample4Y {std::clamp(y + static_cast<int>(std::round(offset4Y)), 0, height - 1)};
                unsigned int color4 {colorBuffer[sample4Y * width + sample4X]};

                // RGB Color B (It looks complex because I stored it as uint32_t which forces me to index color channels like this) //

                unsigned int rgbB_R {(rgbA_R + ((color3 >> 16) & 0xFF) + ((color4 >> 16) & 0xFF)) / 4};
                unsigned int rgbB_G {(rgbA_G + ((color3 >> 8) & 0xFF) + ((color4 >> 8) & 0xFF)) / 4};
                unsigned int rgbB_B {(rgbA_B + (color3 & 0xFF) + (color4 & 0xFF)) / 4};

                float lumaB {getLuma(((rgbB_R << 16) | (rgbB_G << 8) | rgbB_B))};

                unsigned int finalR {};
                unsigned int finalG {};
                unsigned int finalB {};

                if (lumaB < lumaMin || lumaB > lumaMax) {
                    finalR = rgbA_R;
                    finalG = rgbA_G;
                    finalB = rgbA_B;
                } else {
                    finalR = rgbB_R;
                    finalG = rgbB_G;
                    finalB = rgbB_B;
                }

                finalBuffer[y*width+x] = (finalR << 16) | (finalG << 8) | finalB;
            }
        }
    }
}

namespace render{
    // Buffers //

    uint32_t colorBuffer [screenW * screenH] {0};
    uint32_t finalBuffer [screenW * screenH] {0};
    float zBuffer [screenW * screenH] {0};

    // Camera Variables //

    float cameraX {-10};
    float cameraY {2};
    float cameraZ {8};

    float nearPlane {0.01};

    float radian {3.141f/180.0f};

    int screenDistance {25};
    float cameraRotationXZ {180};
    float cameraRotationXY {0};
    float cameraRotationYZ {0};

    float cameraRotationSinXZ {std::sin(cameraRotationXZ*radian)};
    float cameraRotationCosXZ {std::cos(cameraRotationXZ*radian)};

    float cameraRotationSinXY {std::sin(cameraRotationXY*radian)};
    float cameraRotationCosXY {std::cos(cameraRotationXY*radian)};

    float cameraRotationSinYZ {std::sin(cameraRotationYZ*radian)};
    float cameraRotationCosYZ {std::cos(cameraRotationYZ*radian)};

    render::Point3D toCameraSpaceZ(const render::Point3D& point){
        float x {point.x - render::cameraX};
        float y {point.y - render::cameraY};
        float z {point.z - render::cameraZ};

        float tempX {x};
        z = (tempX * render::cameraRotationSinXZ) + (z * render::cameraRotationCosXZ);

        float tempY {y};
        z = (tempY * render::cameraRotationSinYZ) + (z * render::cameraRotationCosYZ);

        return {x, y, z, point.u, point.v};
    }

    void setCameraRotation(float value, int type){
        if(value > 360){
            value = 0;
        }else if(value < 0){
            value = 360;
        }
        switch(type){
            case 1:{
                cameraRotationXZ = value;
                cameraRotationSinXZ = std::sin(cameraRotationXZ*radian);
                cameraRotationCosXZ = std::cos(cameraRotationXZ*radian);
                break;
            }
            case 2:{
                cameraRotationXY = value;
                cameraRotationSinXY = std::sin(cameraRotationXY*radian);
                cameraRotationCosXY = std::cos(cameraRotationXY*radian);
                break;
            }
            case 3:{
                cameraRotationYZ = value;
                cameraRotationSinYZ = std::sin(cameraRotationYZ*radian);
                cameraRotationCosYZ = std::cos(cameraRotationYZ*radian);
                break;
            }
        }
    }

    void moveCameraInDirection(float newZ){
        cameraX += cameraRotationSinXZ * newZ;
        cameraZ += cameraRotationCosXZ * newZ;
    }

    Point2D projectPoint(const Point3D& point){
        float x {point.x};
        float y {point.y};
        float z {point.z};

        float projectedX {};
        float projectedY {};
        bool pointShown {true};

        {
            float tempX {x};
            x = ((x-cameraX)*cameraRotationCosXZ) - ((z-cameraZ)*cameraRotationSinXZ) + cameraX;
            z = ((tempX-cameraX)*cameraRotationSinXZ) + ((z-cameraZ)*cameraRotationCosXZ) + cameraZ;
        }

        {
            float tempY {y};
            y = ((y - cameraY) * cameraRotationCosYZ) - ((z - cameraZ) * cameraRotationSinYZ) + cameraY;
            z = ((tempY - cameraY) * cameraRotationSinYZ) + ((z - cameraZ) * cameraRotationCosYZ) + cameraZ;
        }

        float camZ {z - cameraZ};
        if(camZ <= nearPlane){
            pointShown = false;

        }

        {
            float lineA {y-cameraY};
            float lineB {camZ};
            float lineT {static_cast<float>(screenDistance)};
            float lineQ {(lineA*lineT)/lineB};

            projectedY = lineQ;
        }


        {
            float lineA {x-cameraX};
            float lineB {camZ};
            float lineT {static_cast<float>(screenDistance)};
            float lineQ {(lineA*lineT)/lineB};

            projectedX = lineQ;
        }

        render::Point2D projectedPoint = coordinates::convertCoordinates({projectedX, projectedY, z, pointShown}, screenW, screenH);
        return projectedPoint;
    }
    void resetBuffers(){
        for(int i {0}; i < (screenW * screenH); i++){
            colorBuffer[i] = 0x00000000;
            zBuffer[i] = FLT_MAX;
        }
    }

    float edgeFunction(const Point2D& a, const Point2D& b, const Point2D& c){
        // This is basically Shoelace's Formula to find area of triangle with only its points, but it is signed depending on if the points are clockwise or not & doubled //
        return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x-a.x));
    }

    uint32_t applyIntensity(const uint32_t& color, float intensity){
        unsigned int r {(color >> 16) & 0xFF};
        unsigned int g {(color >> 8) & 0xFF};
        unsigned int b {color & 0xFF};

        r = static_cast<int>(r * intensity);
        g = static_cast<int>(g * intensity);
        b = static_cast<int>(b * intensity);

        r = std::min((unsigned int)(255), r);
        g = std::min((unsigned int)(255), g);
        b = std::min((unsigned int)(255), b);

        return (r << 16) | (g << 8) | b;
    }

    float sampleShadowsSL(vectorFunctions::Vector3D& pixel3DPos, int index, vectorFunctions::Vector3D& normal){
        Point3D worldPixel {pixel3DPos.x, pixel3DPos.y, pixel3DPos.z, 0, 0};
        vectorFunctions::SpotLight& spotLight {vectorFunctions::spotLights[index]};
        shadowFunctions::ShadowMap& shadowMap {shadowFunctions::shadowMaps[index]};

        Point2D lightPixel {shadowFunctions::projectPointSL(worldPixel, spotLight, shadowMap.width, shadowMap.height)};
        int sampleX {static_cast<int>(lightPixel.x)};
        int sampleY {static_cast<int>(lightPixel.y)};

        int neighborAX {};
        int neighborBY {};

        if(sampleX >= shadowMap.width || sampleX < 0 || sampleY >= shadowMap.height || sampleY < 0){
            return 1.0f;
        }

        if(!(sampleX+1 >= shadowMap.width || sampleY+1 >= shadowMap.height)){
            neighborAX = sampleX + 1;
            neighborBY = sampleY + 1;
        }
        float fragWidth {std::abs(shadowMap.buffer[neighborBY*shadowMap.width+sampleX] - shadowMap.buffer[sampleY*shadowMap.width+neighborAX]) + std::abs(shadowMap.buffer[neighborBY*shadowMap.width+sampleX] - shadowMap.buffer[sampleY*shadowMap.width+sampleX])};
        float bias {0.055 + fragWidth};
        if((lightPixel.sZ - spotLight.position.z) < shadowMap.buffer[sampleY*shadowMap.width+sampleX] + bias){
            return 1.0f;
        }else{
            return 0.0f;
        }
    }

    float computeLighting(vectorFunctions::Vector3D& normal, vectorFunctions::Vector3D& pixel3DPos){
        float intensity {};
        for(int i {0}; i < vectorFunctions::directionalLights.size(); i++){
            intensity += std::max(0.0f, vectorFunctions::dotProduct(normal, vectorFunctions::directionalLights[i]));
        }
        for(int i {0}; i < vectorFunctions::spotLights.size(); i++){

            vectorFunctions::Vector3D distanceVector {subtract3D(vectorFunctions::spotLights[i].position, pixel3DPos)};
            float distanceSq {vectorFunctions::dotProduct(distanceVector, distanceVector)};
            float outOfRange {65*(vectorFunctions::spotLights[i].power/100)};
            if(distanceSq > (outOfRange * outOfRange)){
                continue;
            }
            float inverseDistance {1.0f/std::sqrt(distanceSq)};

            vectorFunctions::Vector3D pLLight {vectorFunctions::multiply3D(distanceVector, inverseDistance)}; // Pixel to Light //
            vectorFunctions::Vector3D lPLight {vectorFunctions::multiply3D(pLLight, -1.0f)}; // Light to Pixel //
            float diffusion {std::max(0.0f, vectorFunctions::dotProduct(normal, lPLight))};
            if (diffusion <= 0.0f){
                continue;
            }
            float spot {vectorFunctions::dotProduct(vectorFunctions::spotLights[i].direction, pLLight)};
            float cone {0.0f};

            if (spot > vectorFunctions::spotLights[i].cutOff) {
                cone = 1.0f * (vectorFunctions::spotLights[i].power/100);
            }

            float shadowSample {sampleShadowsSL(pixel3DPos, 0, normal)};
            float falloff {1.0f / (1.0f + distanceSq * 0.02f)};
            intensity += diffusion * cone * falloff * shadowSample;
        }
        for(int i {0}; i < vectorFunctions::pointLights.size(); i++){
            vectorFunctions::Vector3D lPLight {vectorFunctions::fastNormalize(subtract3D(pixel3DPos, vectorFunctions::pointLights[i].position))};
            float diffusion {std::max(0.0f, vectorFunctions::dotProduct(normal, lPLight))};
            if (diffusion <= 0.0f){
                continue;
            }

            vectorFunctions::Vector3D distanceVector {subtract3D(vectorFunctions::pointLights[i].position, pixel3DPos)};
            float distanceSq {vectorFunctions::dotProduct(distanceVector, distanceVector)};
            float falloff {1.0f / distanceSq};

            intensity += falloff * diffusion * vectorFunctions::pointLights[i].power;
        }
        return (intensity * 0.7f) + 0.1f;
    }
    void tileRasterizeModels(){
        std::vector<model::Triangle> clippedTriangles {};
        std::vector<model::Triangle2D> projectedTriangles {};
        for(int i {0}; i < load::loadedModels.size(); i++){
            for(int j {0}; j < load::loadedModels[i].triangles.size(); j++){
                const model::Triangle& tri {load::loadedModels[i].triangles[j].convert(load::loadedModels[i].points, load::loadedModels[i].normals)};
                model::Triangle camTri {tri};

                camTri.p0 = clip::toCameraSpace(tri.p0);
                camTri.p1 = clip::toCameraSpace(tri.p1);
                camTri.p2 = clip::toCameraSpace(tri.p2);

                std::vector<model::Triangle> triangles {clip::clipTriangle(camTri)};

                for(int k {0}; k < triangles.size(); k++){
                    triangles[k].p0 = clip::toWorldSpace(triangles[k].p0);
                    triangles[k].p1 = clip::toWorldSpace(triangles[k].p1);
                    triangles[k].p2 = clip::toWorldSpace(triangles[k].p2);
                    clippedTriangles.push_back(triangles[k]);
                }
            }
        }
        for(int i {0}; i < clippedTriangles.size(); i++){
            const model::Triangle& tri {clippedTriangles[i]};
            model::Triangle2D tri2D {};

            tri2D.p0 = projectPoint(tri.p0);
            tri2D.p1 = projectPoint(tri.p1);
            tri2D.p2 = projectPoint(tri.p2);

            tri2D.n0 = tri.n0;
            tri2D.n1 = tri.n1;
            tri2D.n2 = tri.n2;

            tri2D.hasNormals = tri.hasNormals;
            tri2D.texture = tri.texture;

            projectedTriangles.push_back(tri2D);
        }
        #pragma omp parallel for
        for(int i = 1 ; i <= 12; i++){
            int tileXInc {screenW / 6};
            int tileYInc {screenH / 2};

            int tileMaxX {};
            int tileMaxY {};
            if(i > 6){
                tileMaxX = (i-6)*tileXInc;
                tileMaxY = tileYInc*2;
            }else{
                tileMaxX = i*tileXInc;
                tileMaxY = tileYInc;
            }
            for(int j = 0; j < projectedTriangles.size(); j++){
                const model::Triangle& tri {clippedTriangles[j]};
                const Point2D& p0 {projectedTriangles[j].p0};
                const Point2D& p1 {projectedTriangles[j].p1};
                const Point2D& p2 {projectedTriangles[j].p2};

                bool clockwise {(edgeFunction(p0,p1,p2) > 0) ? true : false};
                if(!clockwise){
                    continue;
                }

                int maxX {static_cast<int>(std::round(std::max({p0.x, p1.x, p2.x})))};
                int maxY {static_cast<int>(std::round(std::max({p0.y, p1.y, p2.y})))};

                int minX {static_cast<int>(std::round(std::min({p0.x, p1.x, p2.x})))};
                int minY {static_cast<int>(std::round(std::min({p0.y, p1.y, p2.y})))};

                minX = std::max(tileMaxX - tileXInc, minX);
                minY = std::max(tileMaxY - tileYInc, minY);

                maxX = std::min(tileMaxX - 1, maxX);
                maxY = std::min(tileMaxY - 1, maxY);

                float area {edgeFunction(p0,p1,p2)};

                float cZ0 {p0.sZ - cameraZ};
                float cZ1 {p1.sZ - cameraZ};
                float cZ2 {p2.sZ - cameraZ};


                for(int y = minY; y <= maxY; y++){
                    for(int x = minX; x <= maxX; x++){
                        Point2D p {x,y};

                        float e0 {edgeFunction(p,p1,p2)}; // Edge Point 1 & Point 2 //
                        float e1 {edgeFunction(p,p2,p0)}; // Edge Point 0 & Point 2 //
                        float e2 {edgeFunction(p,p0,p1)}; // Edge Point 0 & Point 1 //
                        if(e0 >= 0 && e1 >= 0 && e2 >= 0){
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

                            int tileFactor {tri.texture->tileFactor};

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
        }
    }
}

namespace transform{
    void rotateModelXY(render::model::Model& model, float rotate){
        static constexpr float radian {3.141f/180.0f};
        for(int i {0}; i < model.points.size(); i++){
            float rotateRadians {rotate * radian};
            const float cosRotate {std::cos(rotateRadians)};
            const float sinRotate {std::sin(rotateRadians)};

            render::Point3D& point {model.points[i]};
            render::Point3D& centerPoint {model.center};

            float rotatedX {(point.x - centerPoint.x)*cosRotate - (point.y - centerPoint.y)*sinRotate + centerPoint.x};
            float rotatedY {(point.x - centerPoint.x)*sinRotate + (point.y - centerPoint.y)*cosRotate + centerPoint.y};

            point.x = std::round(rotatedX*100.0f)/100.0f;
            point.y = std::round(rotatedY*100.0f)/100.0f;
        }
    }
    void rotateModelXZ(render::model::Model& model, float rotate){
        static constexpr float radian {3.141f/180.0f};
        for(int i {0}; i < model.points.size(); i++){
            float rotateRadians {rotate * radian};
            const float cosRotate {std::cos(rotateRadians)};
            const float sinRotate {std::sin(rotateRadians)};

            render::Point3D& point {model.points[i]};
            render::Point3D& centerPoint {model.center};

            float rotatedX {(point.x - centerPoint.x)*cosRotate - (point.z - centerPoint.z)*sinRotate + centerPoint.x};
            float rotatedZ {(point.x - centerPoint.x)*sinRotate + (point.z - centerPoint.z)*cosRotate + centerPoint.z};

            point.x = std::round(rotatedX*100.0f)/100.0f;
            point.z = std::round(rotatedZ*100.0f)/100.0f;
        }
    }
    void rotateModelYZ(render::model::Model& model, float rotate){
        static constexpr float radian {3.141f/180.0f};
        for(int i {0}; i < model.points.size(); i++){
            float rotateRadians {rotate * radian};
            const float cosRotate {std::cos(rotateRadians)};
            const float sinRotate {std::sin(rotateRadians)};

            render::Point3D& point {model.points[i]};
            render::Point3D& centerPoint {model.center};

            float rotatedY {(point.y - centerPoint.y)*cosRotate - (point.z - centerPoint.z)*sinRotate + centerPoint.y};
            float rotatedZ {(point.y - centerPoint.y)*sinRotate + (point.z - centerPoint.z)*cosRotate + centerPoint.z};

            point.y = std::round(rotatedY*100.0f)/100.0f;
            point.z = std::round(rotatedZ*100.0f)/100.0f;
        }
    }
    void setModelPos(render::model::Model& model, render::Point3D newPos){
        static constexpr float radian {3.141f/180.0f};
        for(int i {0}; i < model.points.size(); i++){
            render::Point3D& point {model.points[i]};
            render::Point3D& centerPoint {model.center};

            point.x += newPos.x - centerPoint.x;
            point.y += newPos.y - centerPoint.y;
            point.z += newPos.z - centerPoint.z;
        }
        model.center = newPos;
    }
}
