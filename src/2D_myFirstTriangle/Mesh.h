#pragma once
#include <GL\glew.h>
#include <glm\glm.hpp>
#include <iostream>
using glm::vec3;
struct Vertex {
public:
	vec3 pos;
	vec3 col;
	vec3 norm;
	Vertex() : pos(vec3()), col(vec3()), norm(vec3()) {}
	Vertex(vec3 position, vec3 color, vec3 normal) : pos(position), col(color), norm(normal) {}
	Vertex(float x, float y, float z, float r, float g, float b, float nx, float ny, float nz) : Vertex(vec3(x, y, z), vec3(r, g, b), vec3(nx, ny, nz)) {}
	Vertex(float x, float y, float z, vec3 color, vec3 normal) : Vertex(vec3(x, y, z), color, normal) {}
	Vertex(float x, float y, float z, vec3 color, float nx, float ny, float nz) : Vertex(vec3(x, y, z), color, vec3(nx, ny, nz)) {}
};
class Mesh {
public:
	Vertex* verts;
	GLuint numVerts;
	GLushort* indices;
	GLuint numIndices;
	GLuint vertexArray;
	static int copyCount;

	Mesh() : verts(0), numVerts(0), indices(0), numIndices(0) {}
	void CleanUp() {
		delete[] verts;
		delete[] indices;
		numIndices = numVerts = 0;
	}

	GLuint VertexBufferSize() const { return numVerts * sizeof(Vertex); }
	GLuint IndexBufferSize() const { return numIndices * sizeof(GLushort); }
	GLuint VertexAndIndexBufferSize() const { return VertexBufferSize() + IndexBufferSize(); }
};

