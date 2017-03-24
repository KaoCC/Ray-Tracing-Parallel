

#include <iostream>


#define __CL_ENABLE_EXCEPTIONS


#include <CL/cl.hpp>


#include "DisplayProcedure.hpp"


int main(int argc, char *argv[]) {
	try {
		std::cerr << "Usage: " << argv[0] << std::endl;
		std::cerr << "Usage: " << argv[0] << " <use CPU devices (0/1)> <use GPU devices (0/1)> \
											 <GPU workgroup size (0=default value or anything x^2)>\
											 <width> <height> <scene file>" << std::endl;

		// It is important to initialize OpenGL before OpenCL
		unsigned int width;
		unsigned int height;
		if (argc == 7) {
			width = atoi(argv[4]);
			height = atoi(argv[5]);
		} else if (argc == 1) {
			width = 800;
			height = 600;
		} else
			exit(-1);

		InitGlut(argc, argv, width, height);

		if (argc == 7)
			rtConfig = new RayTracingConfig(argv[6], width, height,
			(atoi(argv[1]) == 1), (atoi(argv[2]) == 1), atoi(argv[3]));
		else if (argc == 1)
			rtConfig = new RayTracingConfig("../Scene/cornell_test.scn", width, height, true, true, 0);
		else
			exit(-1);

		RunGlut();

	} catch (cl::Error e) {
		std::cerr << "ERROR: " << e.what() << "[" << e.err() << "]" << std::endl;
		return EXIT_FAILURE;
	}

	if (rtConfig) {
		delete rtConfig;
	}

	return EXIT_SUCCESS;


}