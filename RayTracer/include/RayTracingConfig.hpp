
#ifndef _RAYTRACINGCONFIG_HPP_
#define _RAYTRACINGCONFIG_HPP_

#include <vector>
#include <string>

#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_VECTOR
//#define __NO_STD_STRING

#include <CL/cl.hpp>


#include <chrono>
#include <thread>

#include "ComputingUnit.hpp"

#include "Barrier.hpp"

class RayTracingConfig {

public:
	RayTracingConfig(const std::string& sceneFileName, const unsigned int w,
		const unsigned int h, const bool useCPUs, const bool useGPUs,
		const unsigned int forceGPUWorkSize);

	~RayTracingConfig();


	void ReInitScene();
	void ReInit(const bool reallocBuffers);

	void Execute();


	const bool IsProfiling() const;
	const std::vector<ComputingUnit *>& GetComputingItem() const;

	const double GetPerformanceIndex(size_t deviceIndex) const;
	void IncPerformanceIndex(const size_t deviceIndex);
	void DecPerformancefIndex(const size_t deviceIndex);

	void RestartWorkloadProcedure();

	unsigned int selectedDevice;
	char captionBuffer[512];

	unsigned int width{ kDefaultWidth };
	unsigned int height{ kDefaultHeight };
	unsigned int currentSample{ 0 };
	unsigned int *pixels{ nullptr };

	Camera *camera{ nullptr };
	Sphere *spheres{ nullptr };
	unsigned int sphereCount;
	int currentSphere;

private:
	void SetUpOpenCL(const bool useCPUs, const bool useGPUs,
		const unsigned int forceGPUWorkSize);

	void ReadSceneFile(const std::string& fileName);

	void CheckDeviceWorkload();
	void UpdateDeviceWorkload(bool calculateNewLoad);

	void UpdateCamera();

	void ExecuteKernels();

	std::vector<ComputingUnit *> computingUnits;
	std::vector<double> computingUnitsPerfIndex;
	Barrier *threadStartBarrier{ nullptr };
	Barrier *threadEndBarrier{ nullptr };


	std::chrono::system_clock::time_point timeFirstWorkloadUpdate;
	bool workLoadProfilingFlag;

	static const std::string kDefaultKernelPath;
	static const unsigned int kDefaultWidth;
	static const unsigned int kDefaultHeight;

};



#endif