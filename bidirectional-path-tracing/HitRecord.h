#pragma once

#include "vec3.h"

class Material;

struct HitRecord {
	float t;
	Vec3 p;
	Vec3 barycentric;
	Vec3 normal;
	Material* material;
};