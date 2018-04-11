//
//  model.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/23/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>
#include <unordered_map>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "loader.hpp"
#include "model.hpp"

using std::string;
using std::vector;

static std::unordered_map<string, Texture> loadedTexture;

void Model::processNode(const aiNode *node, const aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    // load vertices (position, normal, texCoord)
    vector<Vertex> vertices(mesh->mNumVertices);
    aiVector3D *aiTexCoords = mesh->mTextureCoords[0];
    for (int i = 0; i < vertices.size(); ++i) {
        glm::vec3 position(mesh->mVertices[i].x,
                           mesh->mVertices[i].y,
                           mesh->mVertices[i].z);
        glm::vec3 normal(mesh->mNormals[i].x,
                         mesh->mNormals[i].y,
                         mesh->mNormals[i].z);
        glm::vec2 texCoord(aiTexCoords ? aiTexCoords[i].x : 0.0,
                           aiTexCoords ? aiTexCoords[i].y : 0.0);
        vertices[i] = { position, normal, texCoord };
    }
    
    // load indices
    vector<GLuint> indices;
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }
    
    // load textures
    vector<Texture> textures;
    if (scene->HasMaterials()) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, SPECULAR);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        vector<Texture> reflectionMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, REFLECTION);
        textures.insert(textures.end(), reflectionMaps.begin(), reflectionMaps.end());
    }
    
    return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(const aiMaterial *material,
                                            const aiTextureType aiType,
                                            const TextureType type) {
    vector<Texture> textures;
    for (int i = 0; i < material->GetTextureCount(aiType); ++i) {
        aiString path;
        material->GetTexture(aiType, i, &path);
        string filename(path.C_Str());
        
        auto loaded = loadedTexture.find(filename);
        if (loaded == loadedTexture.end()) {
            Texture texture = { Loader::loadTexture(directory + '/' + filename, type == DIFFUSE), type, filename };
            textures.push_back(texture);
            loadedTexture.insert({ filename, texture });
        } else {
            textures.push_back(loaded->second);
        }
    }
    return textures;
}

Model::Model(const string& objPath, const string& texPath):
directory(texPath) {
    Assimp::Importer importer;
    // other useful options:
    // aiProcess_GenNormals: create normal for vertices
    // aiProcess_SplitLargeMeshes: split mesh when the number of triangles that can be rendered at a time is limited
    // aiProcess_OptimizeMeshes: do the reverse of splitting, merge meshes to reduce drawing calls
    const aiScene *scene = importer.ReadFile(objPath.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    throw std::runtime_error("Failed to import scene: " + string(importer.GetErrorString()));
    
    processNode(scene->mRootNode, scene);
}

void Model::draw(const Shader& shader,
                 const GLuint texOffset,
                 const bool loadTexture) const {
    shader.use();
    std::for_each(meshes.begin(), meshes.end(), [&] (Mesh const& mesh)
    { mesh.draw(shader, texOffset, loadTexture); });
}

void Model::drawInstanced(const Shader& shader,
                          const GLuint amount,
                          const GLuint texOffset,
                          const bool loadTexture) const {
    shader.use();
    std::for_each(meshes.begin(), meshes.end(), [&] (Mesh const& mesh)
    { mesh.drawInstanced(shader, amount, texOffset, loadTexture); });
}
