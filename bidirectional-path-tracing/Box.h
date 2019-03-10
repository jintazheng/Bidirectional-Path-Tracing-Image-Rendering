#pragma once

#include "HitRecord.h"

enum {
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2,
};

class Box {
public:
	Box() {
		mMin = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		mMax = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}
	Box(Vec3 const& min, Vec3 const& max) : mMin(min), mMax(max) {}

	Vec3 Center() {
		return (mMax + mMin) / 2.f;
	}

	void Expand(Vec3 const& pos) {
		mMax = Vec3(fmax(pos.x(), mMax.x()),
			fmax(pos.y(), mMax.y()),
			fmax(pos.z(), mMax.z()));
		mMin = Vec3(fmin(pos.x(), mMin.x()),
			fmin(pos.y(), mMin.y()),
			fmin(pos.z(), mMin.z()));
	}

	void Expand(Box const& other) {
		mMax = Vec3(fmaxf(other.mMax.x(), mMax.x()),
			fmaxf(other.mMax.y(), mMax.y()),
			fmaxf(other.mMax.z(), mMax.z()));
		mMin = Vec3(fminf(other.mMin.x(), mMin.x()),
			fminf(other.mMin.y(), mMin.y()),
			fminf(other.mMin.z(), mMin.z()));
	}

	Vec3 GetSize() {
		return mMax - mMin;
	}

	int GetMajorAxis() {
		float widthx = mMax.x() - mMin.x();
		float widthy = mMax.y() - mMin.y();
		float widthz = mMax.z() - mMin.z();

		if (widthx > widthy && widthx > widthz) {
			return X_AXIS;
		}
		if (widthy > widthx && widthy > widthz) {
			return Y_AXIS;
		}
		return Z_AXIS;
	}

	bool Hit(Ray const& r, float const t_min, float const t_max, HitRecord& rec) const {
		// Precompute
		float const t1 = (mMin.x() - r.origin().x()) * r.invDir.x();
		float const t2 = (mMax.x() - r.origin().x()) * r.invDir.x();
		float const t3 = (mMin.y() - r.origin().y()) * r.invDir.y();
		float const t4 = (mMax.y() - r.origin().y()) * r.invDir.y();
		float const t5 = (mMin.z() - r.origin().z()) * r.invDir.z();
		float const t6 = (mMax.z() - r.origin().z()) * r.invDir.z();

		float tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
		float tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));

		// if tmax < t_min, ray is intersecting AABB, but the whole AABB is behind us
		if (tmax < t_min){
			return false;
		}

		// if tmin > tmax, ray doesn't intersect AABB
		if (tmin > tmax){
			return false;
		}

		// if tmin > t_max, ray intersects but outside out artificial bounds
		if (tmin > t_max) {
			return false;
		}

		rec.t = tmin;
		rec.p = r.point_at_parameter(tmin);
		//rec.normal = ???

		return true;
	}

	void Translate(Vec3 const& trans) {
		mMin += trans;
		mMax += trans;
	}


	Vec3 mMin;
	Vec3 mMax;
};