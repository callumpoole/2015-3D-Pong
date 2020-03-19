#include "MeshGenerator.h"
#include "glm/gtc/constants.hpp"
#include <iostream> //for: memcpy
#include <fstream>
#include <sstream>

#define NUMBER_OF_ARRAY_ELEMENTS(a) sizeof(a)/sizeof(*a)

#pragma region 2D Shapes

Mesh MeshGenerator::MakeTriangle(vec3 color) {
	vec3 a[] = { color, color, color };
	return MakeTriangle(a);
}
Mesh MeshGenerator::MakeTriangle(vec3* color) {
	Mesh m;
	Vertex verts[] = {
		//       X      Y     Z     R G B   NX NY  NZ
		Vertex(-0.1f, -0.1f, 0.0f, color[0], 0, 0, -1),
		Vertex( 0.1f, -0.1f, 0.0f, color[1], 0, 0, -1),
		Vertex( 0.0f,  0.1f, 0.0f, color[2], 0, 0, -1)
	};
	GLushort indices[] = { 0, 1, 2 };
	m.numVerts = NUMBER_OF_ARRAY_ELEMENTS(verts);
	m.numIndices = NUMBER_OF_ARRAY_ELEMENTS(indices);

	//Allocate the memory
	m.verts = new Vertex[m.numVerts];
	m.indices = new GLushort[m.numIndices];
	//Set the values for that memory
	memcpy(m.verts, verts, sizeof(verts));
	memcpy(m.indices, indices, sizeof(indices));

	return m;
}

#pragma endregion


#pragma region 3D Shapes

Mesh MeshGenerator::MakeCube(){
	vec3 a[] = { 
		vec3(1.0, 0.0, 0.0), //Red
		vec3(0.0, 1.0, 0.0), //Green
		vec3(0.0, 0.0, 1.0), //Blue
		vec3(1.0, 0.5, 0.0), //White
		vec3(1.0, 1.0, 0.0), //Yelow
		vec3(0.0, 1.0, 1.0), //Cyan
		vec3(1.0, 0.0, 1.0), //Purple
		vec3(1.0, 0.5, 0.5)  //Pink
	};
	return MakeCube(a);
}
Mesh MeshGenerator::MakeCube(vec3 color){
	vec3 a[8];
	for (int i = 0; i < 8; i++)
		a[i] = color;
	return MakeCube(a);
}
Mesh MeshGenerator::MakeCube(vec3* color){
	Mesh m;
	float n = 1/glm::sqrt(3); //1÷?3
	Vertex verts[] = {
		//       X      Y     Z      R G B    NX  NY  NZ
		Vertex(-0.1f,  0.1f, -0.1f, color[0], -n,  n, -n), // Back face -    Top - Left
		Vertex( 0.1f,  0.1f, -0.1f, color[1],  n,  n, -n), // Back face -    Top - Right
		Vertex(-0.1f, -0.1f, -0.1f, color[2], -n, -n, -n), // Back face - Bottom - Left
		Vertex( 0.1f, -0.1f, -0.1f, color[3],  n, -n, -n), // Back face - Bottom - Right
		Vertex(-0.1f,  0.1f,  0.1f, color[4], -n,  n,  n), //Front face -    Top - Left
		Vertex( 0.1f,  0.1f,  0.1f, color[5],  n,  n,  n), //Front face -    Top - Right
		Vertex(-0.1f, -0.1f,  0.1f, color[6], -n, -n,  n), //Front face - Bottom - Left
		Vertex( 0.1f, -0.1f,  0.1f, color[7],  n, -n,  n)  //Front face - Bottom - Right
	};			
	GLushort indices[] = { 
		4, 7, 6,    4, 5, 7,  //Front face
		0, 3, 1,    0, 2, 3,  //Back face
		0, 4, 2,    4, 6, 2,  //Left face
		1, 3, 5,    5, 3, 7,  //Right face
		1, 4, 0,    1, 5, 4,  //Top face
		2, 6, 7,    2, 7, 3   //Bottom face
	};
	m.numVerts = NUMBER_OF_ARRAY_ELEMENTS(verts);
	m.numIndices = NUMBER_OF_ARRAY_ELEMENTS(indices);

	//Allocate the memory
	m.verts = new Vertex[m.numVerts]; 
	m.indices = new GLushort[m.numIndices];
	//Set the values for that memory
	memcpy(m.verts, verts, sizeof(verts));
	memcpy(m.indices, indices, sizeof(indices));

	return m;
}

Mesh MeshGenerator::MakeHueSphere(int partition, vec3 topCol, vec3 botCol, bool hue){
	Mesh m;
	int div = glm::pow(2, partition + 1); //partion=1, div=4. partition=2, div=8. partition=3, div=16...
	int columnOffsetIndices = 3 + 3 + 6 * (div / 2 - 2); //How many indices are offseted by
	int vertsPerColumnOfSquares = div / 2 - 1; //vertsInColumnOfSquares
	m.numVerts = div*vertsPerColumnOfSquares + 2;
	m.numIndices = div*columnOffsetIndices; //Or 3div(div-2)
	m.verts = new Vertex[m.numVerts]; //derived from div*(div/2-1)+2
	m.indices = new GLushort[m.numIndices]; //Derived from:  6*div*2*(div/4-1) + 3*div*2;
	float radsPerDiv = 2 * glm::pi<float>() / div;
	m.verts[0] = Vertex(0.f, -0.1f, 0.f, topCol, 0, 0, 0); //Bottom Vert
	m.verts[1] = Vertex(0.f, 0.1f, 0.f, hue ? botCol : topCol, 0, 0, 0); //Top Vert

	using glm::cos;
	using glm::sin;
	using glm::degrees;

	int vertexOffset = 2;
	int indexOffset = 0;
	for (int x_z = 0; x_z < div; x_z++) { //Cycling around the x-z plane
		//theta is horizontal angle in radians
		float theta = radsPerDiv * x_z;

		for (int y_xz = 0; y_xz < div/2+1; y_xz++) { //Cycling around the y-xz plane 
			int yNum = -div / 4 + y_xz;

			//phi is vertical angle in radians
			float phi = radsPerDiv * yNum;
			if (y_xz > 0 && y_xz < div/2) { //Range of 1 to div/2
				m.verts[vertexOffset++] = Vertex( //m.verts[2 + y_xz - 1 + x_z*vertsPerColumnOfSquares] could have also been used, but it's computationally better to use a variable for it
					vec3(cos(theta)*cos(phi), sin(phi), sin(theta)*cos(phi)) * 0.1f,
					hue ? GetHue(degrees(theta)) : topCol, vec3(0,0,0));
			}
			 
			//Range of 0 to div/2+1
			if (y_xz == 0) { //If the bottom triangles
				m.indices[indexOffset++] = 0;											//Bottom
				//If it's linked back around, connect to the first vert
				int upRight = x_z == div-1 ? 2 : 2 + vertsPerColumnOfSquares*(x_z + 1);
				int upLeft = 2 + vertsPerColumnOfSquares*(x_z + 0);
				m.indices[indexOffset++] = x_z == div-1 ? 2 : 2 + vertsPerColumnOfSquares*(x_z+1);		//Upwards and Right
				m.indices[indexOffset++] = 2 + vertsPerColumnOfSquares*(x_z+0);							//Upwards and Left
			} else if (y_xz == vertsPerColumnOfSquares) { //If the top triangles
				int downLeft = 2 + vertsPerColumnOfSquares*(x_z + 0) + vertsPerColumnOfSquares - 1;
				int downRight = x_z == div-1 ? 2 + vertsPerColumnOfSquares - 1 : 2 + vertsPerColumnOfSquares*(x_z + 1) + vertsPerColumnOfSquares - 1;
				m.indices[indexOffset++] = 2 + vertsPerColumnOfSquares*(x_z+0) + vertsPerColumnOfSquares-1;								//Downwards and Left
				//If it's linked back around, connect to the first vert
				m.indices[indexOffset++] = x_z == div-1 ? 2+vertsPerColumnOfSquares-1 : 2 + vertsPerColumnOfSquares*(x_z+1) + vertsPerColumnOfSquares-1;	//Downwards and right
				m.indices[indexOffset++] = 1;															//Top
			}
			else if (y_xz == vertsPerColumnOfSquares + 1) {} //Don't connect above the top of the sphere (nothing there)
			else{ //The middle squares (of two triangles)
				int vertOffset = 2 + y_xz - 1 + vertsPerColumnOfSquares*x_z;
				m.indices[indexOffset++] = vertOffset;													//Bottom-Left
				m.indices[indexOffset++] = vertOffset + vertsPerColumnOfSquares     - (x_z == div-1 ? m.numVerts-2 : 0);	//Bottom-Right
				m.indices[indexOffset++] = vertOffset + vertsPerColumnOfSquares + 1 - (x_z == div-1 ? m.numVerts-2 : 0);	//Top-Right
				m.indices[indexOffset++] = vertOffset;													//Bottom-Left
				m.indices[indexOffset++] = vertOffset + vertsPerColumnOfSquares + 1 - (x_z == div-1 ? m.numVerts-2 : 0);	//Top-Right
				m.indices[indexOffset++] = vertOffset + 1;												//Top-Left
			}
		}
	}

	SetNormals(m);
	return m;
}
Mesh MeshGenerator::MakeHueSphere(int partition){
	return MakeHueSphere(partition, vec3(1.f,1.f,1.f), vec3(0.f,0.f,0.f), true);
}
Mesh MeshGenerator::MakeSphere(int partition, vec3 color) {
	return MakeHueSphere(partition, color, color, false);
}

#pragma endregion

vec3 MeshGenerator::GetHue(float angleDeg)
{
	vec3 out;
	if (angleDeg <= 60)
		out = vec3(1, (angleDeg - 0) / 60, 0);
	else if (angleDeg <= 120)
		out = vec3(1 - (angleDeg - 60) / 60, 1, 0);
	else if (angleDeg <= 180)
		out = vec3(0, 1, (angleDeg - 120) / 60);
	else if (angleDeg <= 240)
		out = vec3(0, 1 - (angleDeg - 180) / 60, 1);
	else if (angleDeg <= 300)
		out = vec3((angleDeg - 240) / 60, 0, 1);
	else if (angleDeg <= 360) //Else
		out = vec3(1, 0, 1 - (angleDeg - 300) / 60);

	return out;
}

void MeshGenerator::SetNormals(Mesh& m) {
	//Setup the lighting normals
	for (int i = 0; i < m.numVerts; i++) {
		std::vector<GLushort> indexBlockForVertex;

		for (int j = 0; j < m.numIndices; j++) //Get all of the blocks of 3 indices that the vertex is present
			if (m.indices[j] == i)
				indexBlockForVertex.push_back(glm::floor(j / 3));
		int size = indexBlockForVertex.size();

		vec3 sumOfVectors;
		for (int j = 0; j < size; j++) {
			vec3 a = m.verts[m.indices[indexBlockForVertex[j] * 3]].pos;
			vec3 b = m.verts[m.indices[indexBlockForVertex[j] * 3 + 1]].pos;
			vec3 c = m.verts[m.indices[indexBlockForVertex[j] * 3 + 2]].pos;
			sumOfVectors += glm::normalize(glm::cross(a - b, c - b));
		}
		m.verts[i].norm = glm::normalize(sumOfVectors);
	}
}

std::vector<char> MeshGenerator::ReadAllBytes(char const* filename)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
	std::ifstream::pos_type pos = ifs.tellg();

	std::vector<char>  result(pos);

	ifs.seekg(0, std::ios::beg);
	ifs.read(&result[0], pos);

	return result;
}

Mesh MeshGenerator::LoadObj(const char* filename, vec3 col) {
	Mesh m;
	std::cout << "Loading: " << filename << std::endl;

	std::vector<Vertex> verts;
	std::vector<GLushort> indices;
	std::ifstream File(filename);
	std::string Line;
	std::string Name;
	int vertexCounter = 0, normalCounter = 0, faceCounter = 0;
	const int outputEvery = 500; //The higher this is, the quicker the algorithm will run
	bool fileLoaded = false;
	while (std::getline(File, Line)) {
		fileLoaded = true;
		if (Line == "" || Line[0] == '#')// Skip everything and continue with the next line
			continue;

		std::istringstream LineStream(Line);
		LineStream >> Name;

		if (Name == "v") { // Vertex
			if (++vertexCounter % outputEvery == 0)
				std::cout << "\r\tLoaded " << vertexCounter << "Vertices.";
			vec3 v;
			sscanf(Line.c_str(), "%*s %f %f %f", &v[0], &v[1], &v[2]);

			verts.push_back(Vertex(v, col, vec3()));
		}
		if (Name == "vn") {
			if (normalCounter == 0)
				std::cout << "\r\tLoaded " << vertexCounter << "Vertices." << std::endl;
			if (normalCounter % outputEvery == 0)
				std::cout << "\r\tLoaded " << normalCounter << "Vertex Normals.";
			vec3 n;
			sscanf(Line.c_str(), "%*s %f %f %f", &n[0], &n[1], &n[2]);

			verts[normalCounter++].norm = n;
		}
		if (Name == "f") {
			if (faceCounter == 0)
				std::cout << "\r\tLoaded " << normalCounter << "Vertex Normals." << std::endl;
			if (faceCounter % outputEvery == 0)
				std::cout << "\r\tLoaded " << faceCounter << "Faces.";
			GLushort i[3];
			GLushort j; //Junk, duplicate info
			sscanf(Line.c_str(), "%*s %hu//%hu %hu//%hu %hu//%hu", &i[0], &j, &i[1], &j, &i[2], &j);

			//In this case, slightly more efficient to copy and paste than loop.
			indices.push_back(i[0]-1); //Objs count from 1, not 0.
			indices.push_back(i[1]-1); //Objs count from 1, not 0.
			indices.push_back(i[2]-1); //Objs count from 1, not 0.
			faceCounter++;
		}
	};
	if (fileLoaded) {
		std::cout << "\r\tLoaded " << faceCounter << "Faces." << std::endl;
		std::cout << "\tCopying data into object..." << std::endl;
		m.numVerts = verts.size();
		m.numIndices = indices.size();
		m.verts = new Vertex[m.numVerts];
		m.indices = new GLushort[m.numIndices];
		memcpy(m.verts, &verts[0], m.numVerts * sizeof(Vertex));
		memcpy(m.indices, &indices[0], m.numIndices * sizeof(GLushort));
		std::cout << "\tObject Created!" << std::endl;
		return m;
	} else {
		std::cout << "\tERROR: Loading file, swapping mesh for rainbow Cube..." << std::endl;
		return MakeCube();
	}
}
