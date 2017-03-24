
#include <iostream>
#include <algorithm>

#include "RayTracingConfig.hpp"
#include "Utility.hpp"


const std::string RayTracingConfig::kDefaultKernelPath = "../RayTracer/kernel/rendering_kernel.cl";


RayTracingConfig::RayTracingConfig(const std::string &sceneFileName, const unsigned int w,
	const unsigned int h, const bool useCPUs, const bool useGPUs,
	const unsigned int forceGPUWorkSize):
	selectedDevice(0), width(w), height(h), currentSample(0),
		threadStartBarrier(nullptr), threadEndBarrier(nullptr)
{
	captionBuffer[0] = 0;
	computingUnitsPerfIndex.resize(computingUnits.size(), 1.f);

	ReadSceneFile(sceneFileName);	//need to be changed
	SetUpOpenCL(useCPUs, useGPUs, forceGPUWorkSize);

	// Do the profiling only if there are more than 1 device
	workLoadProfilingFlag = (computingUnits.size() > 1);
	timeFirstWorkloadUpdate = std::chrono::system_clock::now();
}

RayTracingConfig::~RayTracingConfig()
{
	//delete all compting units
	for (size_t i = 0; i < computingUnits.size(); ++i)
		delete computingUnits[i];

	delete[] pixels;
	delete camera;
	delete[] spheres;

	if (threadStartBarrier)
		delete threadStartBarrier;
	if (threadEndBarrier)
		delete threadEndBarrier;
}


void RayTracingConfig::ReadSceneFile(const std::string& fileName) {
	fprintf(stderr, "Reading scene: %s\n", fileName.c_str());

	FILE *f = fopen(fileName.c_str(), "r");
	if (!f) {
		fprintf(stderr, "Failed to open file: %s\n", fileName.c_str());
		exit(-1);
	}

	/* Read the camera position */
	camera = new Camera();
	int c = fscanf(f, "camera %f %f %f  %f %f %f\n",
			&camera->orig.x, &camera->orig.y, &camera->orig.z,
			&camera->target.x, &camera->target.y, &camera->target.z);
	if (c != 6) {
		fprintf(stderr, "Failed to read 6 camera parameters: %d\n", c);
		exit(-1);
	}

	/* Read the sphere count */
	c = fscanf(f, "size %u\n", &sphereCount);
	if (c != 1) {
		fprintf(stderr, "Failed to read sphere count: %d\n", c);
		exit(-1);
	}

	fprintf(stderr, "Scene size: %d\n", sphereCount);

	/* Read all spheres */
	spheres = new Sphere[sphereCount];

	for (unsigned int i = 0; i < sphereCount; i++) {
		Sphere *s = &spheres[i];
		int material;
		int k = fscanf(f, "sphere %f  %f %f %f  %f %f %f  %f %f %f  %d\n",
				&s->rad,
				&s->p.x, &s->p.y, &s->p.z,
				&s->e.x, &s->e.y, &s->e.z,
				&s->c.x, &s->c.y, &s->c.z,
				&material);

		switch (material) {
			case 0:
				s->refl = DIFFuse;
				break;
			case 1:
				s->refl = SPECular;
				break;
			case 2:
				s->refl = REFRactive;
				break;
			default:
				fprintf(stderr, "Failed to parse material type for sphere #%d: %d\n", i, material);
				exit(-1);
				break;
		}

		if (k != 11) {
			fprintf(stderr, "Failed to read sphere #%d: %d\n", i, k);
			exit(-1);
		}
	}

	fclose(f);
}

void RayTracingConfig::SetUpOpenCL(const bool useCPUs, const bool useGPUs,
	const unsigned int forceGPUWorkSize)
{
	// Platform information
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (size_t i = 0; i < platforms.size(); ++i)
		std::cerr << "OpenCL Platform " << i << " : " <<
			platforms[i].getInfo<CL_PLATFORM_VENDOR>().c_str() << std::endl;

	if (platforms.size() == 0)
		throw std::runtime_error("Unable to find an appropiate OpenCL platform");

	// Select the first platform available
	cl::Platform platform = platforms[0];

	// Get the list of devices available on the platform
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

	// Device information
	std::vector<cl::Device> selectedDevices;
	for (size_t i = 0; i < devices.size(); ++i) {
		cl_int type = devices[i].getInfo<CL_DEVICE_TYPE>();
		std::cerr << "OpenCL Device name " << i << " : " <<
				devices[i].getInfo<CL_DEVICE_NAME>().c_str() << std::endl;

		std::string stype;
		switch (type) {
			case CL_DEVICE_TYPE_ALL:
				stype = "TYPE_ALL";
				break;
			case CL_DEVICE_TYPE_DEFAULT:
				stype = "TYPE_DEFAULT";
				break;
			case CL_DEVICE_TYPE_CPU:
				stype = "TYPE_CPU";
				if (useCPUs)
					selectedDevices.push_back(devices[i]);
				break;
			case CL_DEVICE_TYPE_GPU:
				stype = "TYPE_GPU";
				if (useGPUs)
					selectedDevices.push_back(devices[i]);
				break;
			default:
				stype = "TYPE_UNKNOWN";
				break;
		}

		std::cerr << "OpenCL Device type " << i << ": " << stype << std::endl;
		std::cerr << "OpenCL Device units " << i << ": " <<
				devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
	}

	if (selectedDevices.size() == 0)
		throw std::runtime_error("Unable to find an appropiate OpenCL device");
	else {

		// Allocate Computing Units
		threadStartBarrier = new Barrier(selectedDevices.size() + 1);	// Units + Main thread
		threadEndBarrier = new Barrier(selectedDevices.size() + 1);

		for (size_t i = 0; i < selectedDevices.size(); ++i) {
			computingUnits.push_back(new ComputingUnit(
				selectedDevices[i], kDefaultKernelPath, forceGPUWorkSize,
				camera, spheres, sphereCount,
				threadStartBarrier, threadEndBarrier));
		}

		std::cerr << "OpenCL Device used: ";
		for (size_t i = 0; i < computingUnits.size(); ++i)
			std::cerr << "[" << computingUnits[i]->GetDeviceName() << "]";
		std::cerr << std::endl;
	}

	std::cerr << "Create done, width: " << width << ", heigh: " <<height << std::endl;

	pixels = new unsigned int[width * height];

	// Test colors
	for (unsigned int i = 0; i < width * height; ++i)
		pixels[i] = i;

	UpdateDeviceWorkload(true);
	ReInitScene();
	ReInit(false);
}


void RayTracingConfig::ReInitScene()
{
	// Flush everything
	for (size_t i = 0; i < computingUnits.size(); ++i)
		computingUnits[i]->Finish();

	currentSample = 0;

	// Re-download the scene
	for (size_t i = 0; i < computingUnits.size(); ++i)
		computingUnits[i]->UpdateSceneBuffer(spheres);
}


void RayTracingConfig::ReInit(const bool reallocBuffers)
{
	// Flush everything
	for (size_t i = 0; i < computingUnits.size(); ++i)
		computingUnits[i]->Finish();

	currentSample = 0;

	// Check if needed to reallocate buffers
	if (reallocBuffers) {
		delete[] pixels;
		pixels = new unsigned int[width * height];

		// Test colors
		for (unsigned int i = 0; i < width * height; ++i)
			pixels[i] = i;

		// Update devices
		UpdateDeviceWorkload(false);
	}

	UpdateCamera();
}


void RayTracingConfig::Execute()
{
	// Run the kernels
	if (currentSample < 10000) {

		ExecuteKernels();
		currentSample++;

	} else {
		// After the first 10000 samples, continue to execute for more and more time
		auto  startTime = std::chrono::system_clock::now();
		const float k = std::min<unsigned int>(currentSample - 20u, 100u) / 100.f;
		const float thresholdTime = 0.5f * k;

		while (true) {

			ExecuteKernels();
			currentSample++;

			auto endTime = std::chrono::system_clock::now();
			const float elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(endTime - startTime).count();
			if (elapsedTime > thresholdTime)
				break;
		}
	}

	CheckDeviceWorkload();
}

const bool RayTracingConfig::IsProfiling() const 
{
	return workLoadProfilingFlag;
}


void RayTracingConfig::UpdateCamera()
{
	camera->dir = camera->target - camera->orig;
	camera->dir.norm();

	const Vec up(0, 1, 0);
	const float fov = (FLOAT_PI / 180.f) * 45.f;

	camera->x =  camera->dir.cross(up);
	camera->x.norm();
	camera->x = camera->x * (width * fov / height);

	camera->y = camera->x.cross(camera->dir);
	camera->y.norm();
	camera->y = camera->y * fov;

	// Update devices
	for (size_t i = 0; i < computingUnits.size(); ++i)
		computingUnits[i]->UpdateCameraBuffer(camera);
}

const std::vector<ComputingUnit *>& RayTracingConfig::GetComputingItem() const
{
	return computingUnits;
}


void RayTracingConfig::ExecuteKernels()
{
	for (size_t i = 0; i < computingUnits.size(); ++i)
		computingUnits[i]->SetArgs(currentSample);

	// Trigger the rendering threads
	threadStartBarrier->wait();

	// Wait for job done signal
	threadEndBarrier->wait();
}

void RayTracingConfig::CheckDeviceWorkload()
{
	// Check if needed to update the device workload
	auto t = std::chrono::system_clock::now();
	double d = std::chrono::duration_cast<std::chrono::duration<double>>(t - timeFirstWorkloadUpdate).count();

	if (workLoadProfilingFlag && (d > 15.0)) {
		UpdateDeviceWorkload(true);
		workLoadProfilingFlag = false;
	}
}

const double RayTracingConfig::GetPerformanceIndex(size_t index) const
{
	return computingUnitsPerfIndex[index];
}

void RayTracingConfig::UpdateDeviceWorkload(bool calculateNewLoad)
{
	if (calculateNewLoad) {
		// Define how to split the workload
		computingUnitsPerfIndex.resize(computingUnits.size(), 1.f);

		for (size_t i = 0; i < computingUnits.size(); ++i)
			computingUnitsPerfIndex[i] = computingUnits[i]->GetPerformance();
	}

	double totalPerformance = 0.0;
	for (size_t i = 0; i < computingUnits.size(); ++i)
		totalPerformance += computingUnitsPerfIndex[i]; // sum up all the portions

	const unsigned int totalWorkload = width * height;  // total data size

	// Set the workload for each computing unit
	unsigned int workOffset = 0;
	for (size_t i = 0; i < computingUnits.size(); ++i) {

		const unsigned int workLeft = totalWorkload - workOffset;
		unsigned int workAmount;

		if (workLeft <= 0) {

			workOffset = totalWorkload; // After the last pixel
			workAmount = 1;

		} else {
			if (i == computingUnits.size() - 1) {

				// The last device has to complete the work
				workAmount = workLeft;
			} else {
				workAmount = totalWorkload * computingUnitsPerfIndex[i] / totalPerformance;		// balancing
			}
		}

		computingUnits[i]->SetWorkLoad(workOffset, workAmount, width, height, pixels);

		workOffset += workAmount;
	}

	currentSample = 0;
}