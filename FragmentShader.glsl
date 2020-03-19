#version 330
in vec3 passColor;
in vec3 fN; //Interpolated normal of the pixel
in vec3 fL; //light Direction
in vec3 fE; //Relationship between the fragment and the camera
out vec4 outputColor;
void main(){
	vec3 N = normalize(fN);
	vec3 L = normalize(fL);
	vec3 E = normalize(-fE);
	vec3 H = normalize(L+E);

	vec4 final_diffuse = max(dot(N,L), 0.0) * vec4(passColor, 1.0f);
	vec4 final_specular = pow(max(dot(N, H), 0.0), 500) * vec4(1,1,1,1) * .55;
	vec4 final_ambient = vec4(passColor, 1.0f)/3.0f;

	outputColor = final_diffuse + final_ambient + final_specular;
}