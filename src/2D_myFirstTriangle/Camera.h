#pragma once
#include <glm\glm.hpp>
#include "glm\gtx\transform.hpp"
class Camera{
private:

public:
	glm::vec3 pos;
	glm::vec3 dir;
	glm::vec3 up;
	Camera::Camera() : Camera(vec3(0,0,0)) {}
	Camera::Camera(vec3 pos) : pos(pos), dir(glm::vec3(0.f, 0.f, 1.f)), up(glm::vec3(0.f, 0.f, 1.f)) {}
	const void LookAt(float x, float y, float z) { dir = vec3(x,y,z) - pos; }
	const void LookAt(vec3 objPos) { dir = objPos - pos; }

	const glm::mat4 GetViewMatrix() const { return glm::lookAt(pos, pos + dir, up); }
};

