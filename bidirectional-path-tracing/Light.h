#pragma once

#include "util.h"

class Light : public Object {
public:
	virtual Vec3 RandInLight() = 0;
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
};