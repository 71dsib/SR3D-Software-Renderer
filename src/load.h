#ifndef LOAD_H_INCLUDED
#define LOAD_H_INCLUDED

namespace load{
    int loadTexture(std::string filePath, std::vector<render::model::Texture>& loadedTextures);
    int loadOBJModel(std::string filePath, int coordinateSys, bool extractNormals, std::vector<render::model::Model>& loadedModels);
    std::vector<int> loadOBJModelHeirarchy(std::string filePath, int coordinateSys, bool extractNormals, std::vector<render::model::Model>& loadedModels);
}

#endif
