
#include <fstream>
#include <string>

#include <iostream>

#include "ComputingUnit.hpp"

ComputingUnit::ComputingUnit(const cl::Device &dev, const std::string &kernelFileName,
			const unsigned int forceGPUWorkSize,
			Camera *camera, Sphere *spheres,
			const unsigned int sceneSphereCount,
			Barrier *startBarrier, Barrier *endBarrier):
	renderThread(nullptr), threadStartBarrier(startBarrier), threadEndBarrier(endBarrier),
	sphereCount(sceneSphereCount), colorBuffer(nullptr), pixelBuffer(nullptr), seedBuffer(nullptr),
	pixels(nullptr), colors(nullptr), seeds(nullptr), exeUnitCount(0.0), exeTime(0.0)
{

	deviceName = dev.getInfo<CL_DEVICE_NAME >().c_str();


	// Allocate a context with the selected device
	cl::Platform platform = dev.getInfo<CL_DEVICE_PLATFORM>();
	VECTOR_CLASS<cl::Device> devices;
	devices.push_back(dev);
	cl_context_properties cps[3] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform(), 0
	};
	context = cl::Context(devices, cps);

	// Allocate the queue for the device
	cl_command_queue_properties prop = CL_QUEUE_PROFILING_ENABLE;
	queue = cl::CommandQueue(context, dev, prop);


	std::cerr << "Create the kernel" << std::endl;

	// Create the kernel
	std::string src = ReadSources(kernelFileName);

	std::cerr << "Compile the source" << std::endl;

	// Compile sources
	cl::Program::Sources source(1, std::make_pair(src.c_str(), src.length()));
	cl::Program program = cl::Program(context, source);

	std::cerr << "Build" << std::endl;

	try {
		std::vector<cl::Device> buildDevice;
		buildDevice.push_back(dev);
		program.build(buildDevice, "-I.");

		std::string result = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
		std::cerr << "[Device::" << deviceName << "]" << " Compilation result: " << result.c_str() << std::endl;

	} catch(cl::Error e) {

		std::string strError = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
		std::cerr << "[Device::" << deviceName << "]" << " Compilation error:" << std::endl << strError.c_str() << std::endl;

		throw e;
	}

	kernel = cl::Kernel(program, "RadianceGPU");

	kernel.getWorkGroupInfo<size_t>(dev, CL_KERNEL_WORK_GROUP_SIZE, &workGroupSize);
	std::cerr << "[Device::" << deviceName << "]" << " Suggested work group size: " << workGroupSize << std::endl;

	// Force workgroup size if applicable and required
	if ((forceGPUWorkSize > 0) && (dev.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU)) {
		workGroupSize = forceGPUWorkSize;
		std::cerr << "[Device::" << deviceName << "]" << " Forced work group size: " << workGroupSize << std::endl;
	}

	// Create the thread for rendering
	renderThread = new std::thread(std::bind(ComputingUnit::RenderThread, this));

	// Create camera buffer
	cameraBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
			sizeof(Camera), camera);
	
	std::cerr << "[Device::" << deviceName << "] CameraBuffer size: " << (sizeof(Camera) / 1024) << "Kb" << std::endl;
	
	sphereBuffer = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		sizeof(Sphere) * sphereCount, spheres);

	std::cerr << "[Device::" << deviceName << "] SceneBuffer size: " << (sizeof(Sphere) * sphereCount / 1024) << "Kb" << std::endl;
}



ComputingUnit::~ComputingUnit()
{
	if (renderThread) {
		//renderThread->interrupt();
		renderThread->join();

		delete renderThread;
	}



	if (colors)
		delete[] colors;
	if (seeds)
		delete[] seeds;


}


std::string ComputingUnit::ReadSources(const std::string &fileName)
{
	std::fstream file;
	file.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
	file.open(fileName.c_str(), std::fstream::in | std::fstream::binary);

	std::string program(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
	std::cerr << "[Device::" << deviceName << "] Kernel file size " << program.length() << "bytes" << std::endl;

	return program;
}

void ComputingUnit::SetArgs(const unsigned int count) 
{
	currentSample = count;
}

void ComputingUnit::RenderThread(ComputingUnit *computingItem) {
	try {
		while (true) {
			computingItem->threadStartBarrier->wait();

			computingItem->SetKernelArgs();
			computingItem->ExecuteKernel();
			computingItem->ReadPixelBuffer();
			computingItem->FinishExecuteKernel();
			computingItem->Finish();

			computingItem->threadEndBarrier->wait();
		}
	} catch (cl::Error e) {
		std::cerr << "[Device::" << computingItem->GetDeviceName() << "] ERROR: " << e.what() << "(" << e.err() << ")" << std::endl;
	}
}



const std::string& ComputingUnit::GetDeviceName() const
{
	return deviceName;
}

double ComputingUnit::GetPerformance() const 
{
	return ((exeTime == 0.0) || (exeUnitCount == 0.0)) ? 1.0 : (exeUnitCount / exeTime);
}

unsigned int ComputingUnit::GetWorkOffset() const 
{
	return workOffset; 
}

size_t ComputingUnit::GetWorkAmount() const 
{
	return workAmount; 
}

void ComputingUnit::UpdateCameraBuffer(Camera *camera)
{
	queue.enqueueWriteBuffer(cameraBuffer, CL_FALSE, 0, sizeof(Camera), camera);
}

void ComputingUnit::UpdateSceneBuffer(Sphere *spheres)
{
	queue.enqueueWriteBuffer(sphereBuffer, CL_FALSE, 0, sizeof(Sphere) * sphereCount, spheres);
}

void ComputingUnit::Finish()
{
	queue.finish();
}


void ComputingUnit::SetKernelArgs()
{
	kernel.setArg(0, colorBuffer);
	kernel.setArg(1, seedBuffer);
	kernel.setArg(2, sphereBuffer);
	kernel.setArg(3, cameraBuffer);
	kernel.setArg(4, sphereCount);
	kernel.setArg(5, width);
	kernel.setArg(6, height);
	kernel.setArg(7, currentSample);
	kernel.setArg(8, pixelBuffer);
	kernel.setArg(9, workOffset);
	kernel.setArg(10, workAmount);
}

void ComputingUnit::SetWorkLoad(const unsigned int offset, const unsigned int amount,
		const unsigned int screenWidth,	const unsigned int screenHeght,
		unsigned int *screenPixels)
{

	if (colors)
		delete[] colors;
	if (seeds)
		delete[] seeds;

	// parameters
	workOffset = offset;
	workAmount = amount;
	width = screenWidth;
	height = screenHeght;
	pixels = screenPixels;

	std::cerr << "[Device::" << deviceName << "] ";
	std::cerr << "Offset: " <<workOffset << " Amount: " << workAmount << std::endl;

	colors = new Vec[workAmount];
	colorBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		sizeof(Vec) * workAmount, colors);

	std::cerr << "[Device::" << deviceName << "] ColorBuffer size: " << (sizeof(Vec) * workAmount / 1024) << " Kb" << std::endl;

	pixelBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
		sizeof(unsigned int) * workAmount, &pixels[workOffset]);


	std::cerr << "[Device::" << deviceName << "] PixelBuffer size: " << (sizeof(unsigned int) * workAmount / 1024) << " Kb" << std::endl;

	seeds = new unsigned int[workAmount * 2];
	for (size_t i = 0; i < workAmount * 2; ++i) {
		seeds[i] = rand();
		if (seeds[i] < 2)
			seeds[i] = 2;
	}

	seedBuffer = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
		sizeof(unsigned int) * workAmount * 2, seeds);

	std::cerr << "[Device::" << deviceName << "] SeedsBuffer size: " << (sizeof(unsigned int) * 2 * workAmount / 1024) << " Kb" << std::endl;

	currentSample = 0;
}

void ComputingUnit::ReadPixelBuffer() 
{
	queue.enqueueReadBuffer(pixelBuffer, CL_FALSE, 0, sizeof(unsigned int) * workAmount, &pixels[workOffset]);
}


void ComputingUnit::ExecuteKernel()
{
	size_t w = workAmount;
	if (w % workGroupSize != 0) {
		w = (w / workGroupSize + 1) * workGroupSize;
	}

	// This release the old event as well
	kernelExecutionTime = cl::Event();
	queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(w),
		cl::NDRange(workGroupSize), NULL, &kernelExecutionTime);

	exeUnitCount += workAmount;
}

void ComputingUnit::FinishExecuteKernel()
{
	kernelExecutionTime.wait();

	// Check kernel execution time
	cl_ulong t1, t2;
	kernelExecutionTime.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_START, &t1);
	kernelExecutionTime.getProfilingInfo<cl_ulong>(CL_PROFILING_COMMAND_END, &t2);
	exeTime += (t2 - t1) / 1e9;
}

