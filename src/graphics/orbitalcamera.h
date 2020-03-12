/**
*	Simple camera that implements an Orbitalal behaviour. The target point will be (0,0,0).
*
*	@author Alejandro Graciano
*/

#ifndef ORBITAL_CAMERA_H
#define ORBITAL_CAMERA_H

#include <glm/gtc/matrix_transform.hpp>

using glm::vec4;
using glm::vec3;
using glm::mat4;


namespace graphics {
	class OrbitalCamera {
	private:

		float _rotationX, _rotationY, _distance, _translationX, _translationY, _zNear, _zFar, _fov, _aspectRatio;
		mat4 _modelView, _projection;

	public:

		/**
		Constructor
		*/
		OrbitalCamera(float rx = 0, float ry = 0, float dist = 0, float tx = 0, float ty = 0, float fov = 45.0, float aspectRatio = 4.0 / 3.0, float zNear = 0.001f, float zFar = 100.0f);

		/**
		Destructor
		*/
		~OrbitalCamera();

		/**
		Getter methods
		*/
		mat4 getProjection() const { return _projection; }

		float getNear() const { return _zNear; }

		float getFar() const { return _zFar; }

		float getAspectRatio() const { return _aspectRatio; }

		float getFov() const { return _fov; }

		float getRotationX() const { return _rotationX; }

		float getRotationY() const { return _rotationY; }

		float getDistance() const { return _distance; }

		float getTranslationX() const { return _translationX; }

		float getTranslationY() const { return _translationY; }

		/**
		Setter method
		*/
		void setAspectRatio(float aspect) { _aspectRatio = aspect; _projection = glm::perspective(_fov, _aspectRatio, _zNear, _zFar); }

		/**
		Getter method
		*/
		mat4 getModelViewMatrix(mat4 model);

		mat4 getViewMatrix(); // Target is (0,0,0)

		vec3 getPosition();

		/**
		Rotate the camera rx and ry degrees
		*/
		void rotate(const float rx, const float ry);

		/**
		Pan the camera an offset of dx and dy
		*/
		void pan(const float dx, const float dy);

		/**
		Zoom amount offset
		*/
		void zoom(const float amount);



	};

}

#endif