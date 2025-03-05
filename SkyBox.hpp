#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "Shader.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>

namespace gps {
    class SkyBox {
    public:
        SkyBox(); // Constructor
        void Load(std::vector<const GLchar*> cubeMapFaces); // Load cubemap textures
        void Draw(gps::Shader shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix); // Render the skybox
        GLuint GetTextureId(); // Get the cubemap texture ID

    private:
        GLuint skyboxVAO, skyboxVBO; // Vertex Array Object and Vertex Buffer Object
        GLuint cubemapTexture; // ID for the cubemap texture

        GLuint LoadSkyBoxTextures(std::vector<const GLchar*> cubeMapFaces); // Load textures for the cubemap
        void InitSkyBox(); // Initialize the skybox geometry
    };
}

#endif // SKYBOX_HPP
