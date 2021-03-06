// bidirectional-path-tracing.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "vec3.h"
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


int const maxDepth = 5;
int const width = 200;
int const height = 200;
int const samples_per_pixel = 50;
int const thread_count = 25;
int const bytes_per_pixel = 3;


Vec3 Color(Ray const& r, World* world, int depth)
{
	HitRecord rec;
	if (world->mOctree->Hit(r, 0.001f, FLT_MAX, rec)) {
		// Test if we have reached the max reflections
		if (depth > maxDepth) {
			return Vec3(0, 0, 0);
		}

		Vec3 scatteredColor(0, 0, 0);
		Vec3 localColor(0, 0, 0);

		// Check if this is a shadow
		bool inShadow = false;
		Vec3 const shadowDir = world->mLights[0]->RandInLight() - rec.p;
		Ray shadowRay(rec.p, shadowDir.unitVec());
		HitRecord shadowRec;
		inShadow = world->mOctree->Hit(shadowRay, 0.001f, shadowDir.length() - 0.001f, shadowRec);


		if (!inShadow) {
			Solid* solMat = dynamic_cast<Solid*>(rec.material);
			if (solMat) {
				float const diffuseAmount = fmax(0.f, dot(shadowRay.direction(), rec.normal));
				float const specularAmount = fmax(0.f, pow(dot(Reflect(shadowRay.negDirection(), rec.normal).unitVec(), r.negDirection().unitVec()), solMat->mShinyness));
				Vec3 const diffuseColor = diffuseAmount * solMat->mDiffuse;
				Vec3 const specularColor = specularAmount * solMat->mSpecular;
				float const brightness = world->mLights[0]->mIntensity;
				localColor += brightness * (diffuseColor + specularColor);
				localColor.clamp();
			}
			FlatColor* flatMat = dynamic_cast<FlatColor*>(rec.material);
			if (flatMat) {
				localColor = flatMat->mColor;
			}
			//TODO: Metal or dialectric
		}

		float scatterAmount;
		Ray scattered;
		if (rec.material->scatter(r, rec, scatterAmount, scattered)) {
			scatteredColor = scatterAmount * Color(scattered, world, depth + 1);
		}
		Vec3 finalColor = scatteredColor + localColor;
		finalColor.clamp();
		return finalColor;

	} else { // Ray hit nothing (background colors)
		if (world->mLightCount == 0) {
			Vec3 unit_direction = r.direction().unitVec();
			float t = 0.5f * (unit_direction.y() + 1.0f);
			return (1.f - t) * Vec3(1.f, 1.f, 1.f) + t * Vec3(0.5f, 0.7f, 1.0f);
		} else {
			return Vec3(0, 0, 0);
		}
	}
}


void GenerateRows(int const rowBegin, int const rowEnd, int const threadNumber, int8_t* data, World* world, Camera* camera) {
	printf("Thread %d: Start\n", threadNumber);
	for (int jj = rowEnd - 1; jj >= rowBegin; --jj) {
		for (int ii = 0; ii < width; ++ii) {

			Vec3 avgColor(0, 0, 0);
			for (int sample = 0; sample < samples_per_pixel; ++sample) {
				float const u = ((float)ii + RandFloat()) / (float)width;
				float const v = ((float)jj + RandFloat()) / (float)height;

				Ray r = camera->get_ray(u, v);

				avgColor += Color(r, world, 0);
			}

			// Divide by number of samples
			avgColor /= (float)samples_per_pixel;

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
	LoadPreset(&world, &camera, width, height, kShadow);

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