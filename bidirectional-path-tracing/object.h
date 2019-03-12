#pragma once

#include "ray.h"
#include "Box.h"
#include "HitRecord.h"

class Object {
public:
	virtual bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const = 0;
	virtual bool HitBB(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		return mBoundingBox.Hit(r, t_min, t_max, rec);
	}

	virtual void Scale(Vec3 const& scale) {
		mBoundingBox.Scale(scale);
	}

	virtual void Translate(Vec3 const& trans) {
		mBoundingBox.Translate(trans);
	}

	Box mBoundingBox;
};