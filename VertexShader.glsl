#version 330
in vec3 position;
in vec3 colorIn;
in vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 rotationMat; //Used only for rotating the normal vectors
uniform vec4 light;
out vec3 passColor;
out vec3 fN; //Interpolated normal of the pixel
out vec3 fL; //light Direction
out vec3 fE; //Relationship between the fragment and the camera
void main() {
	vec4 pos = vec4(position, 1.0);
	vec4 tempPos =  view * model * pos;

	if (proj == mat4(1.0)) { //If a GUI (projection is identity)
		fN = light.xyz; //If a GUI, set the fN and fL to be the same so the dot product is 1, allowing full diffuse light
	} else { //It's 3D geometry so do proper lighting
		fN = (rotationMat * vec4(normal, 1)).xyz;
	}
	
	fL = light.xyz;

	

	fE = tempPos.xyz; //(view*model*pos).xyz;

	gl_Position = proj * tempPos;

	passColor = colorIn;
}