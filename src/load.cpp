#include "call.h"

namespace load{
    std::vector<render::model::Texture> loadedTextures {};
    std::vector<render::model::Model> loadedModels {};
    int loadTexture(std::string filePath){
        render::model::Texture myTexture {};
        std::ifstream textureFile {filePath};
        if(!textureFile){
            std::cout << "Couldn't Access File \n";
            return -1;
        }
        textureFile >> std::dec >> myTexture.width;
        textureFile >> std::dec >> myTexture.height;
        for(int y {0}; y < myTexture.height; y++){
            for(int x {0}; x < myTexture.width; x++){
                uint32_t color {};
                textureFile >> std::hex >> color;
                myTexture.data.push_back(color);
            }
        }
        loadedTextures.push_back(myTexture);
        return loadedTextures.size()-1;
    }
    int loadOBJModel(std::string filePath, int coordinateSys, bool extractNormals){
        render::model::Model myModel {};
        std::ifstream modelFile {filePath};
        if(!modelFile){
            std::cout << "Couldn't Access File \n";
            return -1;
        }
        std::vector<std::string> fileLines {};
        std::string readLine {};

        std::vector<render::Point3D> vertices {};
        std::vector<vectorFunctions::Vector3D> normals {};
        std::vector<float> textureU {}; // I didn't want to make a new struct just for this //
        std::vector<float> textureV {};

        while(std::getline(modelFile, readLine)){
            fileLines.push_back(readLine);
        }

        for(int i {0}; i < fileLines.size(); i++){
            std::stringstream currentLine {fileLines[i]};
            std::string value {};
            while(std::getline(currentLine, value, ' ')){
                if(value == "v"){
                    float myVertex[3] {};
                    for(int i {0}; i < 3; i++){
                        std::getline(currentLine, value, ' ');
                        myVertex[i] = std::stof(value);
                    }
                    vertices.push_back({myVertex[0], myVertex[1], myVertex[2]*coordinateSys});
                }else if(value == "vt"){
                    std::getline(currentLine, value, ' ');
                    textureU.push_back(std::stof(value));

                    std::getline(currentLine, value, ' ');
                    textureV.push_back(1.0f - std::stof(value));
                }else if(value == "vn"){
                    float myVertex[3] {};
                    for(int i {0}; i < 3; i++){
                        std::getline(currentLine, value, ' ');
                        myVertex[i] = std::stof(value);
                    }
                    normals.push_back({-myVertex[0], myVertex[1], myVertex[2]});
                }else if(value == "f"){
                    std::vector<int> verticesF {};
                    std::vector<int> uvF {};
                    std::vector<int> normalsF {};
                    while(std::getline(currentLine, value, ' ')){
                        std::stringstream valuesA {value};
                        std::string valueA {};

                        std::getline(valuesA, valueA, '/');
                        verticesF.push_back(std::stoi(valueA)-1);

                        std::getline(valuesA, valueA, '/');
                        uvF.push_back(std::stoi(valueA)-1);

                        std::getline(valuesA, valueA, '/');
                        normalsF.push_back(std::stoi(valueA)-1);
                    }
                    if(verticesF.size() == 3){
                        render::model::ModelTriangle myTriangle {};
                        switch(coordinateSys){
                            case 1:{
                                myTriangle.u0 = textureU[uvF[0]];
                                myTriangle.v0 = textureV[uvF[0]];

                                myTriangle.u1 = textureU[uvF[1]];
                                myTriangle.v1 = textureV[uvF[1]];

                                myTriangle.u2 = textureU[uvF[2]];
                                myTriangle.v2 = textureV[uvF[2]];

                                myTriangle.pIndex0 = verticesF[0];
                                myTriangle.pIndex1 = verticesF[1];
                                myTriangle.pIndex2 = verticesF[2];

                                if(extractNormals == false){
                                    break;
                                }

                                myTriangle.nIndex0 = normalsF[0];
                                myTriangle.nIndex1 = normalsF[1];
                                myTriangle.nIndex2 = normalsF[2];

                                break;
                            }
                            case -1:{
                                myTriangle.u0 = textureU[uvF[0]];
                                myTriangle.v0 = textureV[uvF[0]];

                                myTriangle.u1 = textureU[uvF[2]];
                                myTriangle.v1 = textureV[uvF[2]];

                                myTriangle.u2 = textureU[uvF[1]];
                                myTriangle.v2 = textureV[uvF[1]];

                                myTriangle.pIndex0 = verticesF[0];
                                myTriangle.pIndex1 = verticesF[2];
                                myTriangle.pIndex2 = verticesF[1];

                                if(extractNormals == false){
                                    break;
                                }

                                myTriangle.nIndex0 = normalsF[0];
                                myTriangle.nIndex1 = normalsF[2];
                                myTriangle.nIndex2 = normalsF[1];
                                break;
                            }
                            default:{
                                std::cout << "Err. For R. Handed Coordinates, put -1, for L. Handed Coordinates, put 1";
                            }
                        }
                        myTriangle.hasNormals = extractNormals;
                        myModel.triangles.push_back(myTriangle);
                    }else{
                        for(int i {1}; i < verticesF.size() - 1; i++){
                            render::model::ModelTriangle myTriangle {};
                            switch(coordinateSys){
                                case 1:{
                                    myTriangle.u0 = textureU[uvF[0]];
                                    myTriangle.v0 = textureV[uvF[0]];

                                    myTriangle.u1 = textureU[uvF[i]];
                                    myTriangle.v1 = textureV[uvF[i]];

                                    myTriangle.u2 = textureU[uvF[i+1]];
                                    myTriangle.v2 = textureV[uvF[i+1]];


                                    myTriangle.pIndex0 = verticesF[0];
                                    myTriangle.pIndex1 = verticesF[i];
                                    myTriangle.pIndex2 = verticesF[i+1];

                                    if(extractNormals == false){
                                        break;
                                    }

                                    myTriangle.nIndex0 = normalsF[0];
                                    myTriangle.nIndex1 = normalsF[i];
                                    myTriangle.nIndex2 = normalsF[i+1];
                                    break;
                                }
                                case -1:{
                                    myTriangle.u0 = textureU[uvF[0]];
                                    myTriangle.v0 = textureV[uvF[0]];

                                    myTriangle.u1 = textureU[uvF[i+1]];
                                    myTriangle.v1 = textureV[uvF[i+1]];

                                    myTriangle.u2 = textureU[uvF[i]];
                                    myTriangle.v2 = textureV[uvF[i]];


                                    myTriangle.pIndex0 = verticesF[0];
                                    myTriangle.pIndex1 = verticesF[i+1];
                                    myTriangle.pIndex2 = verticesF[i];

                                    if(extractNormals == false){
                                        break;
                                    }

                                    myTriangle.nIndex0 = normalsF[0];
                                    myTriangle.nIndex1 = normalsF[i+1];
                                    myTriangle.nIndex2 = normalsF[i];
                                    break;
                                }
                                default:{
                                    std::cout << "Err. For R. Handed Coordinates, put -1, for L. Handed Coordinates, put 1";
                                }
                            }
                            myTriangle.hasNormals = extractNormals;
                            myModel.triangles.push_back(myTriangle);
                        }
                    }
                }
            }
        }
        myModel.points = vertices;
        if(extractNormals){
            myModel.normals = normals;
        }
        loadedModels.push_back(myModel);
        return loadedModels.size() - 1;
    }
}
