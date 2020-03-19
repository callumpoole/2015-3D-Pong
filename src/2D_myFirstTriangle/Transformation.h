#pragma once
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

using glm::vec3;
using glm::mat4;
class Transformation {
public:
	vec3 position;
	vec3 scale;
	vec3 rotation;

	Transformation() : scale(vec3(1,1,1)){}
	Transformation(vec3 pos) : position(pos), scale(vec3(1, 1, 1)) {}
	Transformation(vec3 pos, vec3 scale) : position(pos), scale(scale) {}
	Transformation(vec3 pos, vec3 scale, vec3 rot) : position(pos), scale(scale), rotation(rot) {}

	float Top(float s)    { return position.y + s*scale.y; }
	float Bottom(float s) { return position.y - s*scale.y; }
	float Left(float s)   { return position.x - s*scale.x; }
	float Right(float s)  { return position.x + s*scale.x; }

	const glm::mat4 GetTransformMatrix() {
		return	glm::scale(
					glm::rotate(
						glm::rotate(
							glm::rotate(
								glm::translate(mat4(), position),
							glm::radians(rotation.z), vec3(0, 0, 1)),
						glm::radians(rotation.y), vec3(0, 1, 0)),
					glm::radians(rotation.x), vec3(1, 0, 0)),
				scale);
	}

	const glm::mat4 GetRotationMatrix() {
		return	glm::rotate(
					glm::rotate(
						glm::rotate(
							mat4(),
						glm::radians(rotation.z), vec3(0, 0, 1)),
					glm::radians(rotation.y), vec3(0, 1, 0)),
				glm::radians(rotation.x), vec3(1, 0, 0));
	}
};

