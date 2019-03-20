#pragma once

#include "vec3.h"
#include "Util.h"

enum MaterialType {
	kDiffuse,
	kSpecular,
	kDielectric,
	kDiffSpec,
	kFlat,
	kLight,
};

class Material {
public:
	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const = 0;

	virtual MaterialType getType() = 0;
};

class Diffuse : public Material {
public:

	Diffuse(Vec3 const& albedo) : mAlbedo(albedo) {}

	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		scattered = Ray(rec.p, (RandInSphere() + rec.normal).unitVec());
		cos_theta = fmax(0.f, dot(rec.normal, scattered.direction()));
		pdf = cos_theta * M_1_PI;
		BRDF = getBRDF();
		return true;
	}

	/*virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		scattered = Ray(rec.p, RandOnHemisphere(rec.normal));
		cos_theta = fmax(0.f, dot(rec.normal, scattered.direction()));
		pdf = 1 / (2 * M_PI);
		BRDF = getBRDF();
		return true;
	}*/

	Vec3 getDirectLighting(Vec3 const& normal, Vec3 const& lightDir, Vec3 const& lightIntensity, float& pdf, float& cos_theta) {
		pdf = 1.f; // TODO: Check that this is the right pdf
		cos_theta = fmax(0.f, dot(normal, lightDir));
		return lightIntensity;
	}

	Vec3 getBRDF() const {
		return mAlbedo * M_1_PI;
	}

	virtual MaterialType getType() {
		return kDiffuse;
	}

	Vec3 mAlbedo;
};

class Metal : public Material {
public:
	Metal(Vec3 const& albedo, float const fuzz) : mAlbedo(albedo), mFuzz(fuzz) {}

	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		Vec3 reflected = Reflect(r_in.direction().unitVec(), rec.normal);
		scattered = Ray(rec.p, reflected + mFuzz * RandInSphere());
		pdf = 1.f;
		cos_theta = fmax(0.f, dot(rec.normal, scattered.direction()));
		BRDF = getBRDF();
		return true;
	}

	Vec3 getBRDF() const {
		return mAlbedo;
	}

	virtual MaterialType getType() {
		return kSpecular;
	}

	Vec3 mAlbedo;
	float mFuzz;
};

class DiffSpec : public Material {
public:

	DiffSpec(Vec3 const& albedo, float const fuzz, float const shininess, float const kD, float const kS) : mAlbedo(albedo), mFuzz(fuzz), mShininess(shininess) {
		float const total = kD + kS;
		if (total <= 1.f) {
			mDiffuseAmount = kD;
			mSpecularAmount = kS;
		} else {
			mDiffuseAmount = kD / total;
			mSpecularAmount = kS / total;
		}
	}

	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		int bounce = getBounceType();
		if (bounce == 1) {
			// Diffuse
			scattered = Ray(rec.p, (RandInSphere() + rec.normal).unitVec());
			cos_theta = fmax(0.f, dot(rec.normal, scattered.direction()));
			pdf = cos_theta;
			BRDF = getBRDFDiffuse();
			return true;
		}
		if (bounce == 2) {
			// Specular
			Vec3 reflected = Reflect(r_in.direction().unitVec(), rec.normal);
			scattered = Ray(rec.p, reflected + mFuzz * RandInSphere());
			pdf = 1.f;
			cos_theta = fmax(0.f, dot(rec.normal, scattered.direction()));
			BRDF = getBRDFSpecular();
			return true;
		}
		// absorbed
		return false;
	}

	Vec3 getDirectLightingDiffuse(Vec3 const& normal, Vec3 const& lightDir, Vec3 const& lightIntensity, float& pdf, float& cos_theta) {
		pdf = 1.f;
		cos_theta = fmax(0.f, dot(normal, lightDir));
		return lightIntensity;
	}

	Vec3 getDirectLightingSpecular(Vec3 const& normal, Vec3 const& negLightDir, Vec3 const& eyeDir, Vec3 const& lightIntensity, float& pdf, float& cos_theta) {
		float const specularAmount = fmax(0.f, pow(dot(Reflect(negLightDir.unitVec(), normal.unitVec()).unitVec(), eyeDir.unitVec()), mShininess));
		cos_theta = 1.f; // Not really sure what to do with these
		pdf = 1.f;
		return lightIntensity * specularAmount;
	}

	Vec3 getBRDFDiffuse() const {
		return mAlbedo / M_PI;
	}

	Vec3 getBRDFSpecular() const {
		return mAlbedo;
	}

	Vec3 getBRDFSpecularDirect() const {
		return Vec3(1, 1, 1) * ((mShininess + 2) / (2 * M_PI));
	}

	virtual MaterialType getType() {
		return kDiffSpec;
	}

	int getBounceType() const {
		float const rand = RandFloat();
		if (rand < mDiffuseAmount) {
			return 1;
		}
		if (rand < mDiffuseAmount + mSpecularAmount) {
			return 2;
		}
		return 0;
	}

	Vec3 mAlbedo;
	float mFuzz;
	float mShininess;
	float mDiffuseAmount;
	float mSpecularAmount;
};

class Dielectric : public Material {
public:
	Dielectric(Vec3 const& albedo, float const ri) : mAlbedo(albedo), ref_idx(ri) {}

	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		Vec3 outward_normal;
		Vec3 reflected = Reflect(r_in.direction().unitVec(), rec.normal);
		Vec3 refracted;
		float ratio_of_indicies;
		float reflect_prob;

		// Calculate the normal based on if the ray is inside or outside the sphere
		if (dot(r_in.direction(), rec.normal) > 0) {
			outward_normal = rec.normal * -1.f;
			ratio_of_indicies = ref_idx;
			cos_theta = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
		} else {
			outward_normal = rec.normal;
			ratio_of_indicies = 1.f / ref_idx;
			cos_theta = -1 * dot(r_in.direction(), rec.normal) / r_in.direction().length();
		}

		// Refract the ray
		if (Refract(r_in.direction(), outward_normal, ratio_of_indicies, refracted)) {
			reflect_prob = Schlick(cos_theta, ref_idx);
		} else {
			scattered = Ray(rec.p, reflected);
			reflect_prob = 1.0f;
		}

		// Decide to reflect or refract randomly
		if (RandFloat() < reflect_prob) {
			scattered = Ray(rec.p, reflected);
		} else {
			scattered = Ray(rec.p, refracted);
		}
		pdf = cos_theta;
		BRDF = getBRDF();
		return true;
	}

	Vec3 getBRDF() const {
		return mAlbedo;
	}

	virtual MaterialType getType() {
		return kDielectric;
	}

	Vec3 mAlbedo;
	float ref_idx;
};

class FlatColor : public Material {
public:
		
	FlatColor(Vec3 const& col) : mColor(col) {}
	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		return false; // Single colors do not scatter
	}

	virtual MaterialType getType() {
		return kFlat;
	}

	Vec3 mColor;
};


class LightMat : public Material {
public:
	LightMat(Vec3 const& intensity) {
		mIntensity = intensity;
	}
	virtual bool scatter(Ray const& r_in, HitRecord const& rec, Ray& scattered, Vec3& BRDF, float& pdf, float& cos_theta) const {
		return false; // Lights don't scatter
	}

	virtual MaterialType getType() {
		return kLight;
	}

	Vec3 mIntensity;
};