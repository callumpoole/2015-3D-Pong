#pragma once
#include "Mesh.h"
#include <vector>

class MeshGenerator
{
private:
	static vec3 GetHue(float angleDeg);
	static void SetNormals(Mesh& m);
	static std::vector<char> ReadAllBytes(char const* filename);
	
public:
	//2D
	static Mesh MakeTriangle(vec3 color);
	static Mesh MakeTriangle(vec3* color);

	//3D
	static Mesh MakeCube();
	static Mesh MakeCube(vec3 color);
	static Mesh MakeCube(vec3* color);
	static Mesh MakeHueSphere(int partition, vec3 topCol, vec3 botCol, bool hue);
	static Mesh MakeHueSphere(int partition);
	static Mesh MakeSphere(int partition, vec3 color);

	static Mesh LoadObj(const char* filename, vec3 col);
};

