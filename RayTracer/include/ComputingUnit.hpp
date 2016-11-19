
#ifndef _COMPUTINGITEM_HPP_
#define _COMPUTINGITEM_HPP_

#include <string>

#define __CL_ENABLE_EXCEPTIONS


#include <CL/cl.hpp>

#include <thread>
#include "Barrier.hpp"

#include "Sphere.hpp"
#include "Camera.hpp"

class ComputingUnit {

public:

	ComputingUnit(const cl::Device &dev, const std::string &kernelFileName,
			const unsigned int forceGPUWorkSize,
			Camera *camera, Sphere *spheres,
			const unsigned int sceneSphereCount,
			Barrier *startBarrier, Barrier *endBarrier);
	~ComputingUnit();

	void SetArgs(const unsigned int count);
	void SetWorkLoad(const unsigned int offset, const unsigned int amount,
		const unsigned int screenWidth,	const unsigned int screenHeght,
		unsigned int *screenPixels);

	void UpdateCameraBuffer(Camera *camera);
	void UpdateSceneBuffer(Sphere *spheres);

	void ResetPerformance();

	void Finish();

	const std::string& GetDeviceName() const;
	double GetPerformance() const;
	unsigned int GetWorkOffset() const;
	size_t GetWorkAmount() const;

private:

	// Thread binding function
	static void RenderThread(ComputingUnit *computingItem);

	std::string ReadSources(const std::string &fileName);
	void SetKernelArgs();

	void ExecuteKernel();
	void FinishExecuteKernel();

	void ReadPixelBuffer();


	std::string deviceName;

	cl::Context context;
	cl::CommandQueue queue;
	cl::Kernel kernel;
	size_t workGroupSize;

	// Thread and barrier for CL kernel
	std::thread *renderThread;
	Barrier *threadStartBarrier;
	Barrier *threadEndBarrier;
	

	unsigned int workOffset;
	unsigned int workAmount;

	// Kernel args
	unsigned int sphereCount;
	unsigned int width;
	unsigned int height;
	unsigned int currentSample;

	// Buffers
	cl::Buffer colorBuffer;
	cl::Buffer pixelBuffer;
	cl::Buffer seedBuffer;
	cl::Buffer sphereBuffer;
	cl::Buffer cameraBuffer;

	// raw 
	Vec *colors {nullptr};
	unsigned int *pixels {nullptr};
	unsigned int *seeds {nullptr};

	// Execution profiling variables
	cl::Event kernelExecutionTime;
	double exeUnitCount;
	double exeTime;

};


#endif