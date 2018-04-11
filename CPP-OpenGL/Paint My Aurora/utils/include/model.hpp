//
//  model.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/23/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef model_hpp
#define model_hpp

#include <vector>
#include <string>
#include <assimp/scene.h>

#include "mesh.hpp"

class Model {
    std::string directory;
    std::vector<Mesh> meshes;
    void processNode(const aiNode *node, const aiScene *scene);
    Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(const aiMaterial *material,
                                              const aiTextureType aiType,
                                              const TextureType type);
public:
    Model(const std::string& objPath, const std::string& texPath = "");
    void draw(const Shader& shader,
              const GLuint texOffset = 0,
              const bool loadTexture = true) const;
    void drawInstanced(const Shader& shader,
                       const GLuint amount,
                       const GLuint texOffset = 0,
                       const bool loadTexture = true) const;
    template<typename Func>
    void appendData(const Func& func) const {
        std::for_each(meshes.begin(), meshes.end(),
                      [&] (Mesh const& mesh) { mesh.appendData(func); });
    }
};

#endif /* model_hpp */
