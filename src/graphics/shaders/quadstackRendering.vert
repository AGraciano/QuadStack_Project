#version 450 core
 
layout (location = 0) in vec3 vertexPosition;

out vec3 entryPoint;
//out vec3 rayDir;


uniform mat4 mvpMatrix;           // Projection * ModelView
uniform vec3 cameraPosition;

void main() {

	entryPoint = vertexPosition;
	//rayDir = normalize(entryPoint - cameraPosition);
    // Convert position to clip coordinates and pass along
    gl_Position = mvpMatrix * vec4(vertexPosition,1.0);
	
}