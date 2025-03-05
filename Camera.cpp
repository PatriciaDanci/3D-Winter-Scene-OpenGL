#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraFrontDirection = glm::normalize(cameraPosition - cameraTarget);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraUp, cameraFrontDirection));
        this->cameraUpDirection = cameraUp;
    }

    //Return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    //Update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == gps::MOVE_FORWARD) {
            cameraPosition += speed * cameraFrontDirection;
        }
        if (direction == gps::MOVE_BACKWARD) {
            cameraPosition -= speed * cameraFrontDirection;
        }
        if (direction == gps::MOVE_LEFT) {
            cameraPosition -= speed * cameraRightDirection;
        }
        if (direction == gps::MOVE_RIGHT) {
            cameraPosition += speed * cameraRightDirection;
        }
        if (direction == gps::MOVE_UP) { // Move up along Y-axis
            cameraPosition.y += speed;
        }
        if (direction == gps::MOVE_DOWN) { // Move down along Y-axis
            cameraPosition.y -= speed;
        }
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    //Update the camera internal parameters following a camera rotate event
    void Camera::rotate(float pitch, float yaw) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f))); // World up
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }



}
