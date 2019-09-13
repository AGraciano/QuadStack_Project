#include "Orbitalcamera.h"
#include <iostream>

#include <glm/glm.hpp>

namespace graphics {

	OrbitalCamera::OrbitalCamera(float rx, float ry, float dist, float tx, float ty, float fov, float aspectRatio, float zNear, float zFar)
		: _rotationX(rx),
		_rotationY(ry),
		_distance(dist),
		_translationX(tx),
		_translationY(ty),
		_zNear(zNear),
		_zFar(zFar),
		_aspectRatio(aspectRatio),
		_fov(fov) {

		_projection = glm::perspective(fov, aspectRatio, zNear, zFar);
	}

	OrbitalCamera::~OrbitalCamera() {}

	mat4 OrbitalCamera::getModelViewMatrix(mat4 model) {

		return getViewMatrix() * model;
	}

	mat4 OrbitalCamera::getViewMatrix() {
		mat4 Tr = glm::translate(mat4(1.0f), glm::vec3(_translationX, _translationY, _distance));
		mat4 Rx = glm::rotate(mat4(1.0f), _rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
		mat4 Ry = glm::rotate(mat4(1.0f), _rotationY, glm::vec3(0.0f, 1.0f, 0.0f));

		return Tr * Rx * Ry;
	}

	void OrbitalCamera::rotate(const float rx, const float ry) {
		_rotationX += rx;
		_rotationY += ry;
	}


	void OrbitalCamera::pan(const float dx, const float dy) {
		_translationX += dx;
		_translationY += dy;
	}


	void OrbitalCamera::zoom(const float amount) {
		_distance += amount;
	}

	vec3 OrbitalCamera::getPosition() {
		mat4 view = getViewMatrix();
		mat4 inverseview = glm::inverse(view);
		vec3 camPos(inverseview[3]);

		return camPos;
	}

}
