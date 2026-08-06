#ifndef LOAD_H_INCLUDED
#define LOAD_H_INCLUDED

namespace load{
    struct AnimatedModel{
        std::vector<int> models {};
    };
    inline std::vector<render::model::Texture> loadedTextures {};
    inline std::vector<render::model::Model> loadedModels {};
}
namespace load{
    int loadTexture(std::string filePath);
    int loadOBJModel(std::string filePath, int coordinateSys, bool extractNormals);
    std::vector<int> loadOBJModelHeirarchy(std::string filePath, int coordinateSys, bool extractNormals);
}

#endif

