#pragma once

#include "util.h"
#include "material.h"

class Light : public Object {
public:
	virtual Vec3 RandInLight() = 0;
	Material* mMaterial;
};

class PointLight : public Light {
public:
	PointLight(Vec3 const& pos, Material* material) {
		mMaterial = material;
		mPosition = pos;
	}

	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		return false;
	}
	virtual Vec3 RandInLight() {
		return mPosition;
	}
	Vec3 mPosition;
};

class BoxLight : public Light {
public:
	BoxLight(Vec3 const& pos, Vec3 const& size, Material* material) {
		mMaterial = material;
		mBoundingBox.Expand(pos + size / 2.f);
		mBoundingBox.Expand(pos - size / 2.f);
	}

	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		bool hit = mBoundingBox.Hit(r, t_min, t_max, rec);
		if (hit) {
			rec.material = mMaterial;
		}
		return hit;
	}
	virtual Vec3 RandInLight() {
		return mBoundingBox.mMax - Vec3(RandFloat(), RandFloat(), RandFloat()) * mBoundingBox.GetSize();
	}
};