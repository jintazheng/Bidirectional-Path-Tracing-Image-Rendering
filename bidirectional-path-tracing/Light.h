#pragma once

#include "util.h"

class Light : public Object {
public:
	virtual Vec3 RandInLight() = 0;
	virtual Vec3 ClosestEdgeOfLight(Vec3) = 0;
	Vec3 mIntensity;
};

class PointLight : public Light {
public:
	PointLight(Vec3 const& pos, Vec3 const& intensity) {
		mIntensity = intensity;
		mPosition = pos;
	}

	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		return false;
	}
	virtual Vec3 RandInLight() {
		return mPosition;
	}
	virtual Vec3 ClosestEdgeOfLight(Vec3 loc) {
		return mPosition;
	}
	Vec3 mPosition;
};

class SphereLight : public Light {
public: 
	SphereLight(Vec3 const& pos, Vec3 const& size, Vec3 const& intensity) {
		mIntensity = intensity;
		mBoundingBox.Expand(pos + size / 2.f);
		mBoundingBox.Expand(pos - size / 2.f);
		mSphere = Sphere(pos, size.x() / 2.f, new FlatColor(intensity));
	}

	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		return mSphere.Hit(r, t_min, t_max, rec);
	}
	virtual Vec3 RandInLight() {
		return mBoundingBox.Center() + RandInSphere() * mBoundingBox.GetSize().x(); // Sphere should have all 3 directions the same size
	}
	virtual Vec3 ClosestEdgeOfLight(Vec3 loc) {
		Vec3 dir = (loc - mSphere.center).unitVec();
		return mSphere.center + loc * mSphere.radius;
	}

	Sphere mSphere;
};

class BoxLight : public Light {
public:
	BoxLight(Vec3 const& pos, Vec3 const& size, Vec3 const intensity) {
		mIntensity = intensity;
		mBoundingBox.Expand(pos + size / 2.f);
		mBoundingBox.Expand(pos - size / 2.f);
	}

	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		return mBoundingBox.Hit(r);
	}
	virtual Vec3 RandInLight() {
		return mBoundingBox.mMax - Vec3(RandFloat(), RandFloat(), RandFloat()) * mBoundingBox.GetSize();
	}

	//Cube mCube;
};