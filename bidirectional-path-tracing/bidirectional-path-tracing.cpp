// bidirectional-path-tracing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "vec3.h"
#include "HitRecord.h"
#include "ray.h"
#include "Object.h"
#include "sphere.h"
#include "accelerationStructure.h"
#include "camera.h"
#include "util.h"
#include "material.h"
#include "triangle.h"
#include "mesh.h"
#include "model.h"
#include "ModelLoader.h"
#include "Octree.h"
#include "Light.h"
#include "World.h"


#include "Presets.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "3rd_party\stb\stb_image_write.h"
// Add ClangFormat


int const maxDepth = 10;
int const width = 300;
int const height = 300;
int const monte_carlos = 50;
int const thread_count = 4;
int const bytes_per_pixel = 3;

Vec3 TracePath(Ray const& r, World* world, int depth) {

	if (depth >= maxDepth) {
		// Max depth has been reached, return black
		return Vec3(0., 0., 0.);
	}

	// Add russian roulette
	/*if (depth >= 2) {

	}*/

	HitRecord rec;
	if (world->mOctree->Hit(r, 0.0001f, FLT_MAX, rec)) {

		// Compute indirect lighting
		Vec3 indirectLighting(0, 0, 0);
		Material* hitMaterial = rec.material;
		Ray sample;
		float pdf_indirect = 1.f;
		float cos_theta_indirect = 0.f;
		Vec3 BRDF(0, 0, 0);
		if (hitMaterial->scatter(r, rec, sample, BRDF, pdf_indirect, cos_theta_indirect)) {
			indirectLighting = TracePath(sample, world, depth + 1);
		}

		if (hitMaterial->getType() == MaterialType::kDiffuse) {
			Diffuse* diffMat = static_cast<Diffuse*>(hitMaterial);
			Light* sampledLight = world->mLights[0];  // In theory choose a random light
			Vec3 const& lightDir = sampledLight->RandInLight() - rec.p;

			// Get the material for the light
			if (sampledLight->mMaterial->getType() != MaterialType::kLight) {
				return Vec3(1, 0, 0); // RED to indicate error
			}
			LightMat* sampleLightMat = static_cast<LightMat*>(sampledLight->mMaterial);
			Vec3 const& directLightIntensity = sampleLightMat->mIntensity;

			// Check if this is a shadow
			Ray shadowRay = Ray(rec.p, lightDir.unitVec());
			HitRecord lightRec;
			bool inShadow = world->mOctree->Hit(shadowRay, 0.001f, lightDir.length() - 0.001f, lightRec);

			// If the only thing hit was the light, we are not in the shadow
			if (inShadow) {
				if(lightRec.material->getType() == MaterialType::kLight) {
					inShadow = false;
				}
			}

			// Compute direct lighting
			if (!inShadow) {
				float pdf_direct = 1.f;
				float cos_theta_direct = 0.f;
				Vec3 const directDiffuseLighting = diffMat->getDirectLighting(rec.normal, shadowRay.direction(), directLightIntensity, pdf_direct, cos_theta_direct);
				Vec3 const finalColor = ((directDiffuseLighting * cos_theta_direct / pdf_direct) + (indirectLighting * cos_theta_indirect / pdf_indirect)) * BRDF / 2.0f;  // divide by 2 because 2 samples were taken
				//finalColor.clamp();
				return finalColor;
			}

			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}

		if (hitMaterial->getType() == MaterialType::kDiffSpec) {
			DiffSpec* diffSpecMat = static_cast<DiffSpec*>(hitMaterial);
			Light* sampledLight = world->mLights[0];  // In theory choose a random light
			Vec3 const& lightDir = sampledLight->RandInLight() - rec.p;

			// Get the material for the light
			if (sampledLight->mMaterial->getType() != MaterialType::kLight) {
				return Vec3(1, 0, 0); // RED to indicate error
			}
			LightMat* sampleLightMat = static_cast<LightMat*>(sampledLight->mMaterial);
			Vec3 const& directLightIntensity = sampleLightMat->mIntensity;

			// Check if this is a shadow
			Ray shadowRay = Ray(rec.p, lightDir.unitVec());
			HitRecord lightRec;
			bool inShadow = world->mOctree->Hit(shadowRay, 0.001f, lightDir.length() - 0.001f, lightRec);

			// If the only thing hit was the light, we are not in the shadow
			if (inShadow) {
				if(lightRec.material->getType() == MaterialType::kLight) {
					inShadow = false;
				}
			}

			// Compute direct lighting
			if (!inShadow) {
				float pdf_direct = 1.f;
				float cos_theta_direct = 0.f;
				int const bounce = diffSpecMat->getBounceType();
				if (bounce == 1) {
					// diffuse
					Vec3 const BRDFDiff = diffSpecMat->getBRDFDiffuse();
					Vec3 const directLighting = diffSpecMat->getDirectLightingDiffuse(rec.normal, shadowRay.direction(), directLightIntensity, pdf_direct, cos_theta_direct);
					Vec3 const finalColor = (((directLighting * cos_theta_direct / pdf_direct) * BRDFDiff) + ((indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF)) / 2.0f;  // divide by 2 because 2 samples were taken
					//finalColor.clamp();
					return finalColor;
				}
				if (bounce == 2) {
					//specular
					Vec3 const BRDFSpec = diffSpecMat->getBRDFSpecularDirect();
					Vec3 const directLighting = diffSpecMat->getDirectLightingSpecular(rec.normal, shadowRay.negDirection(), r.negDirection(), directLightIntensity, pdf_direct, cos_theta_direct);
					Vec3 finalColor = (((directLighting * cos_theta_direct / pdf_direct) * BRDFSpec) + ((indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF)) / 2.0f;  // divide by 2 because 2 samples were taken
					finalColor.clamp();
					return finalColor;
				}
			}

			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}
		if (hitMaterial->getType() == MaterialType::kSpecular) {
			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}
		if (hitMaterial->getType() == MaterialType::kDielectric) {
			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}

		if (hitMaterial->getType() == MaterialType::kLight) {
			LightMat* lightMat = static_cast<LightMat*>(hitMaterial);
			/*if (depth > 0) {
				return Vec3(0, 0, 0); // Lights should not be intersected for indirect lighting
			}*/
			Vec3 finalColor = lightMat->mIntensity;
			//finalColor.clamp();
			return finalColor;
		}

		if (hitMaterial->getType() == MaterialType::kFlat) {
			FlatColor* flatMat = static_cast<FlatColor*>(hitMaterial);
			Vec3 finalColor = flatMat->mColor;
			//finalColor.clamp();
			return finalColor;
		}

		return Vec3(1, 0, 0);  // Return RED to indicate unknown material
	}
	else {  // Ray hit nothing (background colors)
		return Vec3(0, 0, 0); // Black
	}

}

void GenerateRows(int const rowBegin, int const rowEnd, int const threadNumber, int8_t* data, World* world, Camera* camera) {
	printf("Thread %d: Start (row %d - row %d)\n", threadNumber, rowBegin, rowEnd - 1);
	for (int jj = rowEnd - 1; jj >= rowBegin; --jj) {
		for (int ii = 0; ii < width; ++ii) {

			Vec3 avgColor(0, 0, 0);
			for (int sample = 0; sample < monte_carlos; ++sample) {
				float const u = ((float)ii + RandFloat()) / (float)width;
				float const v = ((float)jj + RandFloat()) / (float)height;

				Ray r = camera->get_ray(u, v);

				avgColor += TracePath(r, world, 0);
			}

			// Divide by number of samples
			avgColor /= (float)monte_carlos;

			// Adjust for Gamma
			avgColor = Vec3(sqrt(avgColor[0]), sqrt(avgColor[1]), sqrt(avgColor[2]));

			int offset = (((height - 1) - jj) * width + ii) * bytes_per_pixel;
			data[offset] = (int8_t)(avgColor.r() * 255.99f);
			data[offset + 1] = (int8_t)(avgColor.g() * 255.99f);
			data[offset + 2] = (int8_t)(avgColor.b() * 255.99f);
		}

		int const count = rowEnd - rowBegin;
		printf("Thread %d: %.0f%%\n", threadNumber, float(count - (jj - rowBegin)) / float(count) * 100.f);
	}
}


int main()
{
	srand(time(NULL));

	int8_t* data = new int8_t[width * height * bytes_per_pixel];

	World* world;
	Camera* camera;
	// Load a preset
	LoadPreset(&world, &camera, width, height, kCBoxMaterials);

	// Calculate optimal rows per thread
	int const optimal_row_count = ceil((float)height / (float)thread_count);
	int		  row_count = 0;
	int		  current_row_count;

	// Create a new array of threads
	std::thread thread_array[thread_count];
	for (int ii = 0; ii < thread_count; ++ii)
	{
		//Choose the correct amount of rows for each thread
		int const start = row_count;
		if (start + optimal_row_count <= height) {
			current_row_count = optimal_row_count;
		}
		else {
			current_row_count = height - start;
		}
		row_count += current_row_count;

		//Create the new thread
		thread_array[ii] = std::thread(GenerateRows, start, start + current_row_count, ii, data, world, camera);
	}

	//Wait for all threads to finish
	for (int ii = 0; ii < thread_count; ++ii)
	{
		thread_array[ii].join();
	}

	stbi_write_png("test.png", width, height, bytes_per_pixel, data, width * bytes_per_pixel);

	delete[] data;

	return 0;
}
