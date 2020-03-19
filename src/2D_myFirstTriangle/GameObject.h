#pragma once
#include "Transformation.h"
#include "Mesh.h"
class GameObject {
public:
	Transformation transform;
	Mesh m;
	bool shouldRender = true;
	bool isGUI = false;

	GameObject() {}
	GameObject(Mesh m) : m(m) {}
	GameObject(Mesh m, Transformation t) : m(m), transform(t) {}
};

