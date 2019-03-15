#pragma once

#include "vec3.h"

namespace {
	std::default_random_engine generator(time(NULL));
	std::normal_distribution<float> normalDist(0.f, 1.f);
	std::uniform_real_distribution<float> uniformDist(0.f, 1.f); //[0.f, 1.)
}

template<typename T> 
bool isType(void* inp) {
	return dynamic_cast<T>(inp);
}

float RandFloat() {
	return uniformDist(generator);
}


Vec3 RandOnSphere() {
	Vec3 p;
	p[0] = normalDist(generator);
	p[1] = normalDist(generator);
	p[2] = normalDist(generator);
	p.normalize();
	return p;
}

Vec3 RandOnHemisphere(Vec3 const& normal) {
	Vec3 p = RandOnSphere();
	if (dot(p, normal) < 0.f) {
		p *= -1.f;
	}
	return p;
}

Vec3 RandInSphere() {
	Vec3 p = RandOnSphere();
	float mag = uniformDist(generator);
	p *= cbrt(mag);  // Get a random length
	return p;
}

Vec3 RandInHemisphere(Vec3 const& normal) {
	Vec3 p = RandInSphere();
	if (dot(p, normal) < 0.f) {
		p *= -1;
	}
	return p;
}

Vec3 RandInHemisphereImportance(Vec3& normal) {
	Vec3 p = normal + RandOnSphere();
	p.normalize();
	return p;
}

Vec3 RandOnDisk() {
	Vec3 p;
	p[0] = normalDist(generator);
	p[1] = normalDist(generator);
	p[2] = 0;
	p.normalize();
	return p;
}


Vec3 RandInDisk() {
	Vec3 p = RandOnDisk();
	float mag = uniformDist(generator);
	p *= sqrt(mag);  // Get a random length
	return p;
}

Vec3 RandInDiskNonUniform(float const stdDivSize) {
	Vec3 p;
	p[0] = normalDist(generator) / stdDivSize;
	p[1] = normalDist(generator) / stdDivSize;
	p[2] = 0;
	return p;
}

Vec3 Reflect(Vec3 const& incident, Vec3 const& normal) {
	return incident - 2 * dot(incident, normal) * normal;
}

bool Refract(Vec3 const& incident, Vec3 const& normal, float ratio_of_indicies, Vec3& refracted) {
	Vec3 unitIncident = incident.unitVec();
	float dt = dot(unitIncident, normal);
	float discriminant = 1.0f - ((ratio_of_indicies * ratio_of_indicies) * (1.f - (dt * dt)));
	if (discriminant > 0) {
		refracted = ratio_of_indicies * (unitIncident - (normal * dt)) - (normal * sqrt(discriminant));
		return true;
	}
	return false;
}

float Schlick(float const cosine, float const ref_idx) {
	float r0 = (1.f - ref_idx) / (1.f + ref_idx);
	r0 = r0 * r0;
	return r0 + (1.f - r0) * pow(1.f - cosine, 5.f);
}