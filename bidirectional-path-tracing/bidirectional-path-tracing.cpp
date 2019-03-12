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


int const maxDepth = 15;
int const width = 100;
int const height = 100;
int const monte_carlos = 50;
int const thread_count = 3;
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
		Ray sample;
		float pdf_indirect = 1.f;
		float cos_theta_indirect = 0.f;
		if (rec.material->scatter(r, rec, sample, pdf_indirect, cos_theta_indirect)) {
			indirectLighting = TracePath(sample, world, depth + 1);
		}

		Vec3 const BRDF = rec.material->getBRDF();

		Diffuse* diffMat = dynamic_cast<Diffuse*>(rec.material);
		if (diffMat) {
			Vec3 const lightDir = world->mLights[0]->RandInLight() - rec.p;  // In theory choose a random light
			LightMat* sampleLightMat = dynamic_cast<LightMat*>(world->mLights[0]->mMaterial);
			Vec3 const directLightIntensity = sampleLightMat->mIntensity;

			if (!sampleLightMat) {
				return Vec3(1, 0, 0); // error
			}
			
			// Check if this is a shadow
			Ray shadowRay = Ray(rec.p, lightDir.unitVec());
			HitRecord lightRec;
			bool inShadow = world->mOctree->Hit(shadowRay, 0.001f, lightDir.length() - 0.001f, lightRec);

			// If the only thing hit was the light, we are not in the shadow
			if (inShadow) {
				LightMat* tempMat = dynamic_cast<LightMat*>(lightRec.material);
				if (tempMat) {
					inShadow = false;
				}
			}

			// Compute direct lighting
			Vec3 directDiffuseLighting(0.f, 0.f, 0.f);
			float pdf_direct = 1.f;
			float cos_theta_direct = 0.f;
			if (!inShadow) {
				directDiffuseLighting = diffMat->getDirectLighting(rec.normal, shadowRay.direction(), directLightIntensity, pdf_direct, cos_theta_direct);
				Vec3 finalColor = ((directDiffuseLighting * cos_theta_direct / pdf_direct) + (indirectLighting * cos_theta_indirect / pdf_indirect)) * BRDF / 2.0f;  // divide by 2 because 2 samples were taken
				//finalColor.clamp();
				return finalColor;
			}

			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}

		Metal* metalMat = dynamic_cast<Metal*>(rec.material);
		if (metalMat) {
			return (indirectLighting * cos_theta_indirect / pdf_indirect) * BRDF;
		}

		Dielectric* clearMat = dynamic_cast<Dielectric*>(rec.material);
		if (clearMat) {
			return (indirectLighting * cos_theta_indirect/ pdf_indirect) * BRDF;
		}

		/*Solid* solMat = dynamic_cast<Solid*>(rec.material);
		if (solMat) {
			// Calculate direct lighting
			Vec3 directDiffuseLighting(0, 0, 0);
			Vec3 directSpecularLighting(0, 0, 0);

			// Calculate direct lighting if not in a shadow
			if (!inShadow) {
				float const diffuseAmount = fmax(0.f, dot(shadowRay.direction(), rec.normal)); // cos_theta
				float const specularAmount = fmax(0.f, pow(dot(Reflect(shadowRay.negDirection(), rec.normal).unitVec(), r.negDirection().unitVec()), solMat->mShinyness));
				directDiffuseLighting += directLightIntensity * diffuseAmount;
				directSpecularLighting += directLightIntensity * specularAmount;
			}
	   
			// Calculate indirect lighting
			Vec3 indirectLighting(0, 0, 0);

			// Choose direction and get probability of this direction
			Vec3 const sampleDir = RandOnHemisphere(rec.normal);
			float const invPdf = (2 * M_PI);
			float const cos_theta = dot(sampleDir, rec.normal);
			Ray sampleRay = Ray(rec.p, sampleDir);

			// Trace the new ray
			Vec3 const incomingLight = TracePath(sampleRay, world, depth + 1);
			indirectLighting += incomingLight * cos_theta * invPdf;

			// Calculate the final color of this pixel
			Vec3 const BRDF = solMat->mDiffuse / M_PI;
			Vec3 const emittance = solMat->mEmittance;
			Vec3 finalColor = (directDiffuseLighting + indirectLighting) * BRDF + emittance;
			finalColor.clamp();

			return finalColor;
		}*/

		LightMat* lightMat = dynamic_cast<LightMat*>(world->mLights[0]->mMaterial);
		if (lightMat) {
			/*if (depth > 0) {
				return Vec3(0, 0, 0); // Lights should not be intersected for indirect lighting
			}*/
			Vec3 finalColor = lightMat->mIntensity;
			finalColor.clamp();
			return finalColor;
		}

		FlatColor* flatMat = dynamic_cast<FlatColor*>(rec.material);
		if (flatMat) {
			Vec3 finalColor = flatMat->mColor;
			finalColor.clamp();
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
