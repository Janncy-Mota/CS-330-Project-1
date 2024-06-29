#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <iostream>

#include "ShaderManager.h"
#include "ShapeMeshes.h"

// TEXTURE_INFO structure
struct TEXTURE_INFO
{
    GLuint ID;
    std::string tag;

    TEXTURE_INFO() : ID(0), tag("") {}
};

// LIGHT_SOURCE structure
struct LIGHT_SOURCE
{
    glm::vec3 position;
    glm::vec3 ambientColor;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    float focalStrength;
    float specularIntensity;

    LIGHT_SOURCE()
        : position(0.0f), ambientColor(0.0f), diffuseColor(0.0f), specularColor(0.0f), focalStrength(1.0f), specularIntensity(1.0f) {}
};

// OBJECT_MATERIAL structure
struct OBJECT_MATERIAL
{
    std::string tag;
    glm::vec3 ambientColor;
    float ambientStrength;
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    float shininess;

    OBJECT_MATERIAL()
        : tag(""), ambientColor(0.0f), ambientStrength(0.0f), diffuseColor(0.0f), specularColor(0.0f), shininess(0.0f) {}
};

class SceneManager
{
public:
    SceneManager(ShaderManager* pShaderManager);
    ~SceneManager();

    bool CreateGLTexture(const char* filename, std::string tag);
    void BindGLTextures();
    void DestroyGLTextures();
    int FindTextureID(std::string tag);
    int FindTextureSlot(std::string tag);
    bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);
    void SetTransformations(glm::vec3 scaleXYZ, float XrotationDegrees, float YrotationDegrees, float ZrotationDegrees, glm::vec3 positionXYZ);
    void SetShaderColor(float redColorValue, float greenColorValue, float blueColorValue, float alphaValue);
    void SetShaderTexture(std::string textureTag);
    void SetTextureUVScale(float u, float v);
    void SetShaderMaterial(std::string materialTag);
    void PrepareScene();
    void RenderScene();
    void SetLightColor(float red, float green, float blue, float alpha);
    void SetLightSource(int index, const LIGHT_SOURCE& light);

private:
    ShaderManager* m_pShaderManager;
    ShapeMeshes* m_basicMeshes;
    int m_loadedTextures;
    TEXTURE_INFO m_textureIDs[128]; // Assume a max of 128 textures
    std::vector<OBJECT_MATERIAL> m_objectMaterials;
    LIGHT_SOURCE m_lightSources[4]; // Array to hold light sources
};
