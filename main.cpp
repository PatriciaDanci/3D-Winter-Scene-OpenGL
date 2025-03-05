#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include "glm/glm.hpp" //core glm functionality
#include "glm/gtc/matrix_transform.hpp" //glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp" //glm extension for computing inverse matrices
#include "glm/gtc/type_ptr.hpp" //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
int glWindowWidth = 1900;
int glWindowHeight = 1050;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 2048 * 3;
const unsigned int SHADOW_HEIGHT = 2048 * 3;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
glm::vec3 lightColorPoint;
GLuint lightColorLoc;
GLuint lightColorLocPoint;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 5.5f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.5f;

GLboolean pressedKeys[1024];

bool transparentLake = false;
bool isNightMode = false;

//pt animatii
float currentTime;
float deltaTime;
float lastFrameTime;

// models
gps::Model3D winterWonderland;
gps::Model3D lake;
gps::Model3D pineTree;
gps::Model3D cottage;
gps::Model3D deer;
gps::Model3D screenQuad;
gps::Model3D lightCube;
gps::Model3D nanosuit;
gps::Model3D ground;
gps::Model3D lakeup;
gps::Model3D bark;
gps::Model3D leaves;
gps::Model3D shelter;
gps::Model3D Teeter;

gps::Model3D raindrop;
struct Raindrop {
    glm::vec3 position;
    glm::vec3 velocity;
    float lifespan;
};

std::vector<Raindrop> raindrops;
bool isRaining = false;

struct SmokeParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float transparency;
    float lifespan;
};
std::vector<SmokeParticle> smokeParticles;


float angleY = 0.0f;
GLfloat lightAngle;
float teeterRotationAngle = 0.0f;

//skybox 
gps::SkyBox mySkyBox;
gps::SkyBox nightSkyBox;

//shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;
gps::Shader lakeShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) {
        return;
    }

    glViewport(0, 0, width, height);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    GLint projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}


GLenum renderMode = GL_FILL;

std::vector<glm::vec3> tourPositions;
std::vector<glm::vec3> tourTargets;
float tourT = 0.0f; // Interpolation factor
bool isTourActive = false;
float tourSpeed = 0.008f; 
int positionIndex = 0; 
void startAutomatedTour() {
    //y-axis constant ca sa para sa se misca pe o linie kinda
    tourPositions = {
    glm::vec3(0.0f, 3.0f, 10.0f),   // Start position near the lake
    glm::vec3(-5.0f, 3.0f, 5.0f),  // Close to the deer
    glm::vec3(-5.0f, 3.0f, 19.0f), // Near the teeter
    glm::vec3(-23.149f, 3.0f, 33.97f), // Close to the shelter
    glm::vec3(-22.9f, 3.0f, 11.8f), // Among the trees
    glm::vec3(24.0f, 3.0f, -6.0f),  // Close to the cottage
    glm::vec3(6.925f, 3.0f, -21.88f), // Another tree group
    glm::vec3(0.0f, 3.0f, 10.0f)    // Loop back to the starting point
    };

    tourTargets = {
        glm::vec3(-10.0f, 3.0f, 5.0f),  // Looking toward the deer
        glm::vec3(-5.0f, 3.0f, 19.0f),  // Looking at the teeter
        glm::vec3(-23.149f, 3.0f, 33.97f), // Looking at the shelter
        glm::vec3(-22.9f, 3.0f, 11.8f), // Focus on a tree
        glm::vec3(24.0f, 3.0f, -6.0f),  // Looking at the cottage
        glm::vec3(6.925f, 3.0f, -21.88f), // Another tree
        glm::vec3(0.0f, 3.0f, 10.0f),   // Focus back to the lake
        glm::vec3(-10.0f, 3.0f, 5.0f)   // Loop back to the deer
    };

    tourT = 0.0f;
    positionIndex = 0;
    isTourActive = true;
}

void updateAutomatedTour() {
    if (!isTourActive || positionIndex >= tourPositions.size() - 1)
        return;

    glm::vec3 currentPos = glm::mix(tourPositions[positionIndex], tourPositions[positionIndex + 1], tourT);
    glm::vec3 currentTarget = glm::mix(tourTargets[positionIndex], tourTargets[positionIndex + 1], tourT);

    myCamera.cameraPosition = currentPos;
    myCamera.cameraTarget = currentTarget;
    myCamera.cameraFrontDirection = glm::normalize(currentTarget - currentPos);
    myCamera.cameraRightDirection = glm::normalize(glm::cross(myCamera.cameraUpDirection, myCamera.cameraFrontDirection));

    tourT += tourSpeed;
    if (tourT >= 1.0f) {
        tourT = 0.0f; 
        positionIndex++; 
    }

    if (positionIndex >= tourPositions.size() - 1) {
        isTourActive = false;
    }
}


GLuint smokeTexture;

void initSmokeTexture() {
    glGenTextures(1, &smokeTexture);
    glBindTexture(GL_TEXTURE_2D, smokeTexture);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("models/assets/smoke.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cerr << "Failed to load smoke texture" << std::endl;
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void generateSmokeParticles(float deltaTime) {
    
    for (int i = 0; i < 5; i++) {
        SmokeParticle newParticle;
        //newParticle.position = glm::vec3(20.5f, 9.3f, -0.9f); //coords chimney
        newParticle.position = glm::vec3(
            20.5f + ((rand() % 50 - 25) / 100.0f), 
            9.3f,
            -0.9f + ((rand() % 50 - 25) / 100.0f) 
        );

        newParticle.velocity = glm::vec3(
            (rand() % 200 - 100) / 300.0f, 
            (rand() % 150 + 100) / 400.0f,
            (rand() % 200 - 100) / 300.0f  
        );
        newParticle.size = 0.2f;
        newParticle.transparency = 1.0f; //incep cu el negru iar pe parcurs se disipa
        newParticle.lifespan = 5.0f; 
        smokeParticles.push_back(newParticle);
    }

    // Update existing particles
    for (auto it = smokeParticles.begin(); it != smokeParticles.end();) {
        it->position += it->velocity * deltaTime;
        it->size += deltaTime * 0.1f;          // Particles grow
        it->transparency -= deltaTime * 0.2f;  // Particles fade
        it->lifespan -= deltaTime;

        if (it->lifespan <= 0.4f || it->transparency <= 0.0f) {
            it = smokeParticles.erase(it); 
        }
        else {
            ++it;
        }
    }
}


void drawSmokeParticles(gps::Shader shader) {
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.useShaderProgram();

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isSmoke"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, smokeTexture);

    for (const auto& particle : smokeParticles) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), particle.position);
        model = glm::scale(model, glm::vec3(particle.size));
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform1f(glGetUniformLocation(shader.shaderProgram, "smokeTransparency"), particle.transparency);

        screenQuad.Draw(shader); 
    }

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isSmoke"), 0); 
    //glDisable(GL_BLEND);
}



int nbr = 50;

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;

            if (key == GLFW_KEY_1) { 
                renderMode = GL_FILL;
                glPolygonMode(GL_FRONT_AND_BACK, renderMode);
            }
            if (key == GLFW_KEY_2) { 
                renderMode = GL_LINE;
                glPolygonMode(GL_FRONT_AND_BACK, renderMode);
            }
            if (key == GLFW_KEY_3) { 
                renderMode = GL_POINT;
                glPolygonMode(GL_FRONT_AND_BACK, renderMode);
            }
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        static bool fogEnabled = false;
        fogEnabled = !fogEnabled;

        myBasicShader.useShaderProgram();
        GLint fogEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled");
        if (fogEnabledLoc != -1) { 
            glUniform1i(fogEnabledLoc, fogEnabled);
        }
        else {
            std::cerr << "Failed to locate fogEnabled uniform." << std::endl;
        }
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) { // Enable directional light
        myBasicShader.useShaderProgram();
        GLint dirLightEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "dirLightEnabled");
        glUniform1i(dirLightEnabledLoc, GL_TRUE);

        lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f); 
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "ambientLight"), 1, glm::value_ptr(ambientLight));

        std::cout << "Directional light enabled (day mode)." << std::endl;
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        isNightMode = !isNightMode;

        myBasicShader.useShaderProgram();

        GLint dirLightEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "dirLightEnabled");
        glUniform1i(dirLightEnabledLoc, !isNightMode);

        GLint additionalPointLightsEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "additionalPointLightsEnabled");
        glUniform1i(additionalPointLightsEnabledLoc, isNightMode);

        if (isNightMode) {
            glm::vec3 ambientLight = glm::vec3(0.1f, 0.1f, 0.2f);
            glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "ambientLight"), 1, glm::value_ptr(ambientLight));

            std::cout << "Night mode activated. Additional point lights enabled." << std::endl;
        }
        else {
            glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
            glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "ambientLight"), 1, glm::value_ptr(ambientLight));

            std::cout << "Day mode activated. Additional point lights disabled." << std::endl;
        }
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        static bool pointLightEnabled = false;
        pointLightEnabled = !pointLightEnabled; 

        myBasicShader.useShaderProgram();
        GLint pointLightEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightEnabled");
        glUniform1i(pointLightEnabledLoc, pointLightEnabled ? GL_TRUE : GL_FALSE);

        if (pointLightEnabled) {
            lightColorPoint = glm::vec3(0.0f, 0.0f, 5.0f); // Blue light
            std::cout << "Point light enabled." << std::endl;
        }
        else {
            lightColorPoint = glm::vec3(0.0f, 0.0f, 0.0f); // Black light
            std::cout << "Point light disabled." << std::endl;
        }

        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColorPoint"), 1, glm::value_ptr(lightColorPoint));
    }


    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        startAutomatedTour();
    }

	if (key == GLFW_KEY_9 && action == GLFW_PRESS) {
		isRaining = !isRaining;
        nbr = 50; //reset the num of rain particles
        if (isRaining) {
			std::cout << "It's raining!" << std::endl;
		}
		else {
			std::cout << "The rain has stopped." << std::endl;
		}
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		nbr += 10;
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		nbr -= 10;
	}

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        static bool cursorVisible = true;
        cursorVisible = !cursorVisible;

        if (cursorVisible) {
            glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "Cursor enabled." << std::endl;
        }
        else {
            glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            std::cout << "Cursor disabled." << std::endl;
        }
    }

}

void updateRaindrops(float deltaTime) {
    if (isRaining) {
        if (raindrops.size() < 5000) {
            for (int i = 0; i < nbr; i++) {
                Raindrop newRaindrop;
                newRaindrop.position = glm::vec3(
                    rand() % 100 - 20, 
                    20.0f,            
                    rand() % 100 - 20  
                );
                newRaindrop.velocity = glm::vec3(0.0f, -10.0f, 0.0f); 
                newRaindrop.lifespan = 3.0f; 
                raindrops.push_back(newRaindrop);
            }
        }

        for (auto it = raindrops.begin(); it != raindrops.end();) {
            it->position += it->velocity * deltaTime;
            it->lifespan -= deltaTime;
            if (it->position.y < -1.5f || it->lifespan <= 0.0f) {
                it = raindrops.erase(it); 
            }
            else {
                ++it;
            }
        }
    }
}

void drawRaindrops(gps::Shader shader) {
    shader.useShaderProgram();
    for (const auto& drop : raindrops) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), drop.position);
        model = glm::scale(model, glm::vec3(0.05f)); 
        glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        raindrop.Draw(shader);
    }
}


// Mouse callback function
bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float yaw1 = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch1 = 0.0f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        firstMouse = false;
        lastX = xpos;
        lastY = ypos;
    }

    float x_offset = xpos - lastX;
    float y_offset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;

    yaw1 += x_offset;
    pitch1 += y_offset;

    if (pitch1 > 89.0f)
        pitch1 = 89.0f;
    if (pitch1 < -89.0f)
        pitch1 = -89.0f;

    myCamera.rotate(pitch1, yaw1);
}

float pitch = 0.0f; // Initial vertical angle (up/down rotation)
float yaw = -90.0f; // Initial horizontal angle (left/right rotation)

void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_R]) { // Move up
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_F]) { // Move down
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_Q]) { // Rotate left
        yaw -= 1.0f;
        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_E]) { 
        yaw += 1.0f; 
        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_T]) { // Rotate upward (backflip)
        pitch += 1.0f;
        if (pitch > 89.0f) pitch = 89.0f;

        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (pressedKeys[GLFW_KEY_G]) { // Rotate downward (frontflip)
        pitch -= 1.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        myCamera.rotate(pitch, yaw);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    if (isTourActive) {
        updateAutomatedTour();
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }


}

bool initOpenGLWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Project", NULL, NULL);
    if (!glWindow) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);

    glfwMakeContextCurrent(glWindow);
    glfwSwapInterval(1);

#if not defined (__APPLE__)
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    return true;
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);
}

void initOpenGLState() {
    //glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f); 
    glViewport(0, 0, retina_width, retina_height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LESS); 
    glDisable(GL_CULL_FACE); 
    glCullFace(GL_BACK); 
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glFrontFace(GL_CCW); 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initObjects() {
    winterWonderland.LoadModel("models/assets/doarground.obj");//schimba cu wholeground.obj
    lake.LoadModel("models/assets/lake.obj");
    lakeup.LoadModel("models/assets/lakeup.obj");
    raindrop.LoadModel("models/assets/raindrop.obj");
    Teeter.LoadModel("models/assets/Teeter03.obj");
    pineTree.LoadModel("models/assets/Pine_Tree.obj");
	bark.LoadModel("models/assets/bark.obj");
	leaves.LoadModel("models/assets/leaves.obj");
	shelter.LoadModel("models/assets/bonfire.obj");
    cottage.LoadModel("models/assets/cottage.obj");
    deer.LoadModel("models/assets/Deer.obj");
    nanosuit.LoadModel("models/assets/nanosuit/nanosuit.obj");
    ground.LoadModel("models/assets/ground/ground.obj");
    screenQuad.LoadModel("models/assets/quad/quad.obj");
    /*std::vector<const GLchar*> faces = {
        "models/assets/skybox/IceRiver/posx.jpg",
        "models/assets/skybox/IceRiver/negx.jpg",
        "models/assets/skybox/IceRiver/posy.jpg",
        "models/assets/skybox/IceRiver/negy.jpg",
        "models/assets/skybox/IceRiver/posz.jpg",
        "models/assets/skybox/IceRiver/negz.jpg"
    };*/
    std::vector<const GLchar*> faces = {
        "models/assets/skybox/nou/bluecloud_ft.jpg",
        "models/assets/skybox/nou/bluecloud_bk.jpg",
        "models/assets/skybox/nou/bluecloud_up.jpg",
        "models/assets/skybox/nou/bluecloud_dn.jpg",
        "models/assets/skybox/nou/bluecloud_rt.jpg",
        "models/assets/skybox/nou/bluecloud_lf.jpg"
    };
    mySkyBox.Load(faces);
    std::vector<const GLchar*> facesn = {
        "models/assets/skybox/night/px.png",
        "models/assets/skybox/night/nx.png",
        "models/assets/skybox/night/py.png",
        "models/assets/skybox/night/ny.png",
        "models/assets/skybox/night/pz.png",
        "models/assets/skybox/night/nz.png",
    };
	nightSkyBox.Load(facesn);
        
}

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    myBasicShader.useShaderProgram();

    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    skyboxShader.useShaderProgram();

    depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
    depthMapShader.useShaderProgram();

    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();

    lakeShader.loadShader("shaders/lake.vert", "shaders/lake.frag");
    lakeShader.useShaderProgram();
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    projection = glm::perspective(glm::radians(45.0f),
        (float)retina_width / (float)retina_height,
        0.5f, 5000.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Fog Parameters
    GLint fogEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnabled");
    glUniform1i(fogEnabledLoc, 0); // Start with fog disabled

    GLint fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    glUniform4f(fogColorLoc, 0.5f, 0.5f, 0.5f, 1.0f); // Gray fog

    GLint fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    glUniform1f(fogDensityLoc, 0.002f); // Example fog density

    //3 point lights
    glm::vec3 pointLight1Position = glm::vec3(10.0f, 10.0f, -45.0f);  // Light 1
    glm::vec3 pointLight1Color = glm::vec3(10.0f, 0.0f, 0.0f);        // Red light

    glm::vec3 pointLight2Position = glm::vec3(-30.0f, 20.0f, 5.0f);  // Light 2
    glm::vec3 pointLight2Color = glm::vec3(0.0f, 10.0f, 0.0f);        // Green light

    glm::vec3 pointLight3Position = glm::vec3(19.545f, 25.0f, 34.879f); // Light 3
    glm::vec3 pointLight3Color = glm::vec3(0.0f, 0.0f, 10.0f);        // Blue light


    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight1Position"), 1, glm::value_ptr(pointLight1Position));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight1Color"), 1, glm::value_ptr(pointLight1Color));

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight2Position"), 1, glm::value_ptr(pointLight2Position));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight2Color"), 1, glm::value_ptr(pointLight2Color));

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight3Position"), 1, glm::value_ptr(pointLight3Position));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLight3Color"), 1, glm::value_ptr(pointLight3Color));


    GLint additionalPointLightsEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "additionalPointLightsEnabled");
    glUniform1i(additionalPointLightsEnabledLoc, GL_FALSE);

    GLint dirLightEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "dirLightEnabled");
    glUniform1i(dirLightEnabledLoc, true); // Directional light enabled by default

    //set the light direction (direction towards the light)
    lightDir = glm::vec3(1.0f, 1.0f, 1.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

    GLint pointLightEnabledLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightEnabled");
    glUniform1i(pointLightEnabledLoc, GL_FALSE); // Point light disabled by default

    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    lightColorPoint = glm::vec3(10.0f, 0.0f, 0.0f); //white light
    lightColorLocPoint = glGetUniformLocation(myBasicShader.shaderProgram, "lightColorPoint");
    glUniform3fv(lightColorLocPoint, 1, glm::value_ptr(lightColorPoint));

    glm::vec3 pointLightPosition = glm::vec3(0.0f, 10.0f, 0.0f); // Above the scene
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPosition"), 1, glm::value_ptr(pointLightPosition));

    //smoke
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isSmoke"), 0); // Default to no smoke
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "smokeTransparency"), 1.0f); // Full transparency for smoke

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    glGenFramebuffers(1, &shadowMapFBO);

    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach the texture to the fbo
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    glm::vec3 lightPosition = glm::vec3(25.0f, 20.0f, 5.0f);
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightView = glm::lookAt(glm::vec3(lightRotation * glm::vec4(lightPosition, 1.0f)), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat nearPlane = 0.1f, farPlane = 300.0f;
    glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, nearPlane, farPlane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass)
{

    shader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, -1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    winterWonderland.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.63f, 0.0f));
    model = glm::scale(model, glm::vec3(2.2f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        glUniform1f(glGetUniformLocation(shader.shaderProgram, "time"), currentTime);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyWave"), true);
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        lake.Draw(shader);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyWave"), false);
    }

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.53f, 0.0f));
    model = glm::scale(model, glm::vec3(2.2f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        glUniform1i(glGetUniformLocation(shader.shaderProgram, "transparentLake"), true);
        lakeup.Draw(shader);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "transparentLake"), false);
    }

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-23.149f, -1.5f, 33.97f));
	model = glm::rotate(model, glm::radians(110.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(2.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    shelter.Draw(shader);

    //****trees
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1f(glGetUniformLocation(shader.shaderProgram, "time"), currentTime);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-11.12f, -1.5f, 19.1f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-11.12f, -1.5f, 19.1f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, -1.5f, 11.8f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, -1.5f, 11.8f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(6.925f, -1.5f, -21.88f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(6.925f, -1.5f, -21.88f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-11.602f, -1.5f, 2.7516f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-11.602f, -1.5f, 2.7516f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-13.493f, -1.5f, 7.3101f));
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-13.493f, -1.5f, 7.3101f));
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-23.477f, -1.5f, 25.825f));
    model = glm::scale(model, glm::vec3(2.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-23.477f, -1.5f, 25.825f));
    model = glm::scale(model, glm::vec3(2.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
	glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

	//-------------- noi
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-14.558f, -1.5f, -28.595f));
    model = glm::scale(model, glm::vec3(2.2f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-14.558f, -1.5f, -28.595f));
    model = glm::scale(model, glm::vec3(2.2f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1f(glGetUniformLocation(shader.shaderProgram, "time"), currentTime);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-20.524f, -1.5f, -25.879f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    bark.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-20.524f, -1.5f, -25.879f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1f(glGetUniformLocation(shader.shaderProgram, "time"), currentTime);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), true);

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    leaves.Draw(shader);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "applyTreeWind"), false);

    //--------------
    //****cottage
    model = glm::translate(glm::mat4(1.0f), glm::vec3(24.0f, -1.4f, -6.0f));
    model = glm::rotate(model, glm::radians(145.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    cottage.Draw(shader);

	//****teeter
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, -0.7f, 19.0f));
    model = glm::rotate(model, glm::radians(145.0f), glm::vec3(0.0f, 1.0f, 0.0f)); 
    model = glm::rotate(model, glm::radians(teeterRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); 
    model = glm::scale(model, glm::vec3(0.003f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    Teeter.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, -1.5f, 5.0f));
    model = glm::scale(model, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    deer.Draw(shader);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // do not send the normal matrix if we are rendering in the depth map
    if (!depthPass)
    {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    //mySkyBox.Draw(skyboxShader, view, projection);
	//nightSkyBox.Draw(skyboxShader, view, projection);
    if (isNightMode) {
        nightSkyBox.Draw(skyboxShader, view, projection);
    }
    else {
        mySkyBox.Draw(skyboxShader, view, projection);
    }


}

float deltaTimeAnim = 0.0f; 
float lastFrameAnim = 0.0f;

void renderScene() {

    // depth maps creation pass
    //TODO - Send the light-space transformation matrix to the depth map creation shader and
    //		 render the scene in the depth map

    currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

	deltaTimeAnim = currentTime - lastFrameAnim;
	lastFrameAnim = currentTime;

    lightAngle += deltaTime * 1.0f; // rotim lumina ca sa urmarim umbrele aici

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawObjects(depthMapShader, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render depth map on screen - toggled with the M key

    if (showDepthMap) {
        glViewport(0, 0, retina_width, retina_height);

        glClear(GL_COLOR_BUFFER_BIT);

        screenQuadShader.useShaderProgram();

        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
    }
    else {

        // final scene rendering pass (with shadows)

        glViewport(0, 0, retina_width, retina_height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        myBasicShader.useShaderProgram();

        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));


        teeterRotationAngle += deltaTimeAnim * 20.0f; 
        if (teeterRotationAngle > 15.0f || teeterRotationAngle < -15.0f) {
            deltaTimeAnim = -deltaTimeAnim; 
        }

        drawObjects(myBasicShader, false);

        if (isRaining) {
            // Update raindrops positions
            updateRaindrops(deltaTime);

            // Render the raindrops
            drawRaindrops(myBasicShader);
        }

        generateSmokeParticles(deltaTime);
        drawSmokeParticles(myBasicShader);


    }
}

void cleanup() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glfwDestroyWindow(glWindow);
    glfwTerminate();
}

int main(int argc, const char* argv[]) {
    if (!initOpenGLWindow()) {
        glfwTerminate();
        return -1;
    }

    initOpenGLState();
    initObjects();
    initShaders();
    initUniforms();
    initFBO();

    lastFrameTime = glfwGetTime();

    glCheckError();

    while (!glfwWindowShouldClose(glWindow)) {
        processMovement();
        renderScene();
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    cleanup();

    return 0;
}
