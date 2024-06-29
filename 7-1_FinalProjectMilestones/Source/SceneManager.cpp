///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Janncy Mota - SNHU Student / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, June. 8th, 2024
///////////////////////////////////////////////////////////////////////////////
#include "SceneManager.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
    const char* g_LightColorName = "lightColor"; // Added this line
    const char* g_MaterialAmbientColor = "material.ambientColor";
    const char* g_MaterialDiffuseColor = "material.diffuseColor";
    const char* g_MaterialSpecularColor = "material.specularColor";
    const char* g_MaterialShininess = "material.shininess";
    const char* g_LightPosition = "light.position";
    const char* g_ViewPosition = "viewPos";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/

SceneManager::SceneManager(ShaderManager* pShaderManager)
    : m_pShaderManager(pShaderManager), m_basicMeshes(new ShapeMeshes()), m_loadedTextures(0)
{
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
    m_pShaderManager = NULL;
    delete m_basicMeshes;
    m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0;
    int height = 0;
    int colorChannels = 0;
    GLuint textureID = 0;

    // indicate to always flip images vertically when loaded
    stbi_set_flip_vertically_on_load(true);

    // try to parse the image data from the specified image file
    unsigned char* image = stbi_load(
        filename,
        &width,
        &height,
        &colorChannels,
        0);

    // if the image was successfully read from the image file
    if (image)
    {
        std::cout << "Successfully loaded image: " << filename << ", width: " << width << ", height: " << height << ", channels: " << colorChannels << std::endl;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // if the loaded image is in RGB format
        if (colorChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        // if the loaded image is in RGBA format - it supports transparency
        else if (colorChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
            return false;
        }

        // generate the texture mipmaps for mapping textures to lower resolutions
        glGenerateMipmap(GL_TEXTURE_2D);

        // free the image data from local memory
        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        // register the loaded texture and associate it with the special tag string
        m_textureIDs[m_loadedTextures].ID = textureID;
        m_textureIDs[m_loadedTextures].tag = tag;
        m_loadedTextures++;

        return true;
    }

    std::cout << "Could not load image: " << filename << std::endl;

    // Error loading the image
    return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glDeleteTextures(1, &m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
    for (const auto& texture : m_textureIDs)
    {
        if (texture.tag == tag)
            return texture.ID;
    }
    return -1;
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        if (m_textureIDs[i].tag == tag)
            return i;
    }
    return -1;
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), positionXYZ) *
        glm::rotate(glm::mat4(1.0f), glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), scaleXYZ);

    if (m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, model);
    }
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
    float redColorValue,
    float greenColorValue,
    float blueColorValue,
    float alphaValue)
{
    glm::vec4 currentColor(redColorValue, greenColorValue, blueColorValue, alphaValue);

    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
    }
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);
        int textureSlot = FindTextureSlot(textureTag);
        if (textureSlot != -1)
        {
            glActiveTexture(GL_TEXTURE0 + textureSlot);
            glBindTexture(GL_TEXTURE_2D, m_textureIDs[textureSlot].ID);
            m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
        }
    }
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
    }
}

/***********************************************************
 *  SetLightColor()
 *
 *  This method is used for setting the light color
 ***********************************************************/
void SceneManager::SetLightColor(float red, float green, float blue, float alpha)
{
    glm::vec4 lightColor(red, green, blue, alpha);
    if (m_pShaderManager)
    {
        m_pShaderManager->setVec4Value(g_LightColorName, lightColor);
    }
}

/***********************************************************
 *  SetLightSource()
 *
 *  This method is used for setting the light source parameters
 ***********************************************************/
void SceneManager::SetLightSource(int index, const LIGHT_SOURCE& light)
{
    if (index < 0 || index >= 4)
        return;

    m_lightSources[index] = light;

    std::string lightIndex = "lightSources[" + std::to_string(index) + "].";
    m_pShaderManager->setVec3Value(lightIndex + "position", light.position);
    m_pShaderManager->setVec3Value(lightIndex + "ambientColor", light.ambientColor);
    m_pShaderManager->setVec3Value(lightIndex + "diffuseColor", light.diffuseColor);
    m_pShaderManager->setVec3Value(lightIndex + "specularColor", light.specularColor);
    m_pShaderManager->setFloatValue(lightIndex + "focalStrength", light.focalStrength);
    m_pShaderManager->setFloatValue(lightIndex + "specularIntensity", light.specularIntensity);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
    // only one instance of a particular mesh needs to be
    // loaded in memory no matter how many times it is drawn
    // in the rendered 3D scene

    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadSphereMesh(); // Ensure you have this method to load a sphere mesh

    // Load textures
    CreateGLTexture("textures/bark.jpg", "bark");
    CreateGLTexture("textures/grass.jpg", "grass");
    CreateGLTexture("textures/water.jpg", "water");
    CreateGLTexture("textures/leaves.jpg", "leaves");
    CreateGLTexture("textures/sky.jpg", "sky"); // Load sky texture

    // Set up light sources
    LIGHT_SOURCE light1;
    light1.position = glm::vec3(-10.0f, 50.0f, -20.0f);
    light1.ambientColor = glm::vec3(0.3f, 0.15f, 0.0f); // Warmer soft ambient light
    light1.diffuseColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange diffuse light
    light1.specularColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange specular light
    light1.focalStrength = 0.2f;
    light1.specularIntensity = 0.2f;
    SetLightSource(0, light1);

    LIGHT_SOURCE light2;
    light2.position = glm::vec3(-8.0f, 8.0f, -22.0f);
    light2.ambientColor = glm::vec3(0.3f, 0.15f, 0.0f); // Warmer soft ambient light
    light2.diffuseColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange diffuse light
    light2.specularColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange specular light
    light2.focalStrength = 0.2f;
    light2.specularIntensity = 0.2f;
    SetLightSource(1, light2);

    LIGHT_SOURCE light3;
    light3.position = glm::vec3(10.0f, 9.0f, -18.0f);
    light3.ambientColor = glm::vec3(0.3f, 0.15f, 0.0f); // Warmer soft ambient light
    light3.diffuseColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange diffuse light
    light3.specularColor = glm::vec3(1.0f, 0.6f, 0.0f); // Warmer orange specular light
    light3.focalStrength = 0.2f;
    light3.specularIntensity = 0.2f;
    SetLightSource(2, light3);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 scaleXYZ;
    float XrotationDegrees = 0.0f;
    float YrotationDegrees = 0.0f;
    float ZrotationDegrees = 0.0f;
    glm::vec3 positionXYZ;

    m_pShaderManager->use();

    // Set the light and view positions
    glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f); // Example view position
    m_pShaderManager->setVec3Value(g_ViewPosition, viewPos);

    // Set light properties
    for (int i = 0; i < 3; ++i) {
        std::string lightIndex = "lightSources[" + std::to_string(i) + "].";
        m_pShaderManager->setVec3Value(lightIndex + "position", m_lightSources[i].position);
        m_pShaderManager->setVec3Value(lightIndex + "ambientColor", m_lightSources[i].ambientColor);
        m_pShaderManager->setVec3Value(lightIndex + "diffuseColor", m_lightSources[i].diffuseColor);
        m_pShaderManager->setVec3Value(lightIndex + "specularColor", m_lightSources[i].specularColor);
    }

    // Enable lighting
    m_pShaderManager->setIntValue(g_UseLightingName, true);

    // Set material properties for the plane
    glm::vec3 ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;

    m_pShaderManager->setVec3Value(g_MaterialAmbientColor, ambientColor);
    m_pShaderManager->setVec3Value(g_MaterialDiffuseColor, diffuseColor);
    m_pShaderManager->setVec3Value(g_MaterialSpecularColor, specularColor);
    m_pShaderManager->setFloatValue(g_MaterialShininess, shininess);

    // Grass Floor Plane
    scaleXYZ = glm::vec3(25.0f, 5.0f, 36.0f);
    XrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, -1.0f, 0.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderTexture("grass");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawPlaneMesh();

    // Water Plane
    scaleXYZ = glm::vec3(25.0f, 1.0f, 2.0f);
    XrotationDegrees = 0.0f;
    positionXYZ = glm::vec3(0.0f, -0.5f, 0.0f); // Aligning the water plane with the grass plane
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderTexture("water");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawPlaneMesh();

    // Sun 1
    scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);
    positionXYZ = glm::vec3(-10.0f, 10.0f, -20.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange color
    m_basicMeshes->DrawSphereMesh(); // Assuming a sphere mesh is available

    // Sun 2
    scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);
    positionXYZ = glm::vec3(-8.0f, 8.0f, -22.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange color
    m_basicMeshes->DrawSphereMesh(); // Assuming a sphere mesh is available

    // Sun 3
    scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
    positionXYZ = glm::vec3(10.0f, 9.0f, -18.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange color
    m_basicMeshes->DrawSphereMesh(); // Assuming a sphere mesh is available

    // Mountain 1
    scaleXYZ = glm::vec3(10.0f, 5.0f, 10.0f);
    positionXYZ = glm::vec3(-10.0f, 0.0f, -20.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(0.5f, 0.35f, 0.05f, 1.0f); // Brownish color
    m_basicMeshes->DrawConeMesh();

    // Mountain 2
    scaleXYZ = glm::vec3(8.0f, 4.0f, 8.0f);
    positionXYZ = glm::vec3(10.0f, 0.0f, -15.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(0.55f, 0.4f, 0.1f, 1.0f); // Slightly different brownish color
    m_basicMeshes->DrawConeMesh();

    // Mountain 3
    scaleXYZ = glm::vec3(12.0f, 6.0f, 12.0f);
    positionXYZ = glm::vec3(0.0f, 0.0f, -25.0f);
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderColor(0.6f, 0.45f, 0.15f, 1.0f); // Another shade of brown
    m_basicMeshes->DrawConeMesh();

    // Trees
    std::vector<glm::vec3> treePositions = {
        glm::vec3(10.0f, -1.0f, 5.0f),
        glm::vec3(15.0f, -1.0f, 8.0f),
        glm::vec3(18.0f, -1.0f, 3.0f),
        glm::vec3(-10.0f, -1.0f, 5.0f),
        glm::vec3(-15.0f, -1.0f, 8.0f),
        glm::vec3(-18.0f, -1.0f, 3.0f),
        glm::vec3(10.0f, -1.0f, -5.0f),
        glm::vec3(15.0f, -1.0f, -8.0f),
        glm::vec3(18.0f, -1.0f, -3.0f),
        glm::vec3(-10.0f, -1.0f, -5.0f),
        glm::vec3(-15.0f, -1.0f, -8.0f),
        glm::vec3(-18.0f, -1.0f, -3.0f)
    };

    for (const auto& treePos : treePositions)
    {
        // Tree Trunk
        scaleXYZ = glm::vec3(0.5f, 3.0f, 0.5f);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, treePos) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scaleXYZ);
        m_pShaderManager->setMat4Value("model", model);
        SetShaderTexture("bark");
        SetTextureUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawCylinderMesh();

        // Tree Cone
        scaleXYZ = glm::vec3(2.0f, 3.0f, 2.0f);
        glm::vec3 conePos = treePos + glm::vec3(0.0f, 1.5f, 0.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, conePos) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scaleXYZ);
        m_pShaderManager->setMat4Value("model", model);
        SetShaderTexture("leaves");
        SetTextureUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawConeMesh();
    }

    // Additional trees for more variety
    std::vector<glm::vec3> additionalTreePositions = {
        glm::vec3(-3.0f, -1.0f, 2.0f),
        glm::vec3(3.0f, -1.0f, -2.0f),
        glm::vec3(-7.0f, -1.0f, 3.0f),
        glm::vec3(7.0f, -1.0f, -3.0f),
        glm::vec3(-2.0f, -1.0f, -4.0f),
        glm::vec3(2.0f, -1.0f, 4.0f),
        glm::vec3(-6.0f, -1.0f, -3.0f),
        glm::vec3(6.0f, -1.0f, 3.0f),
        glm::vec3(15.0f, -1.0f, 10.0f),
        glm::vec3(-15.0f, -1.0f, -10.0f),
        glm::vec3(20.0f, -1.0f, 12.0f),
        glm::vec3(-20.0f, -1.0f, -12.0f)
    };

    for (const auto& treePos : additionalTreePositions)
    {
        // Tree Trunk
        scaleXYZ = glm::vec3(0.5f, 3.0f, 0.5f);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, treePos) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scaleXYZ);
        m_pShaderManager->setMat4Value("model", model);
        SetShaderTexture("bark");
        SetTextureUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawCylinderMesh();

        // Tree Cone
        scaleXYZ = glm::vec3(2.0f, 3.0f, 2.0f);
        glm::vec3 conePos = treePos + glm::vec3(0.0f, 1.5f, 0.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, conePos) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scaleXYZ);
        m_pShaderManager->setMat4Value("model", model);
        SetShaderTexture("leaves");
        SetTextureUVScale(1.0f, 1.0f);
        m_basicMeshes->DrawConeMesh();
    }

    // Plane aligned with Grass Plane
    scaleXYZ = glm::vec3(25.0f, 5.0f, 10.0f);
    positionXYZ = glm::vec3(0.0f, 9.0f, -36.0f); // Adjust the position to align with the grass plane
    XrotationDegrees = 90.0f; // Rotate to face up as a background
    SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
    SetShaderTexture("sky");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawPlaneMesh();
}
