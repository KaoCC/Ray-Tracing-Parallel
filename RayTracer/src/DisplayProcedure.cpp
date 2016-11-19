
#include <iostream>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <string>

#include <chrono>

#include <GL/glut.h>

#include "RayTracingConfig.hpp"
#include "ComputingUnit.hpp"
#include "DisplayProcedure.hpp"
#include "Utility.hpp"


static bool printHelp = true;
static bool showWorkLoad = true;

static double totalElapsedTime = 0.0;


static bool isMouseTracking = false;
static std::pair<int, int> mousePos;


RayTracingConfig *rtConfig;

static void UpdateRendering() {

	int startSampleCount = rtConfig->currentSample;
	if (startSampleCount == 0)
		totalElapsedTime = 0.0;

	auto startTime = std::chrono::system_clock::now();
	rtConfig->Execute();
	auto endTime = std::chrono::system_clock::now();
	const double elapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime).count();
	totalElapsedTime += elapsedTime;

	const int samples = rtConfig->currentSample - startSampleCount;
	const double sampleSec = samples * rtConfig->height * rtConfig->width / elapsedTime;

	sprintf(rtConfig->captionBuffer, "[Rendering time %.3f sec (pass %d)][Avg. sample/sec %.1fK][Instant sample/sec %.1fK]",
			elapsedTime, rtConfig->currentSample,
			(rtConfig->currentSample) * (rtConfig->height) * (rtConfig->width) / totalElapsedTime / 1000.f,
			sampleSec / 1000.f);
}

static void PrintString(void *font, const std::string& str) {

	for (int i = 0; i < str.length(); ++i)
		glutBitmapCharacter(font, str[i]);
}

static void PrintHelpAndItems() {

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.f, 0.5f);
	glRecti(10, 80, 630, 180);
	glDisable(GL_BLEND);

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(320 - glutBitmapLength(GLUT_BITMAP_9_BY_15, (unsigned char *)"Help & Devices") / 2, 420);
	//PrintString(GLUT_BITMAP_9_BY_15, "Help & Devices");

	// Help
	//glRasterPos2i(60, 390);
	//PrintString(GLUT_BITMAP_9_BY_15, "h - toggle Help");
	//glRasterPos2i(60, 375);
	//PrintString(GLUT_BITMAP_9_BY_15, "arrow Keys - rotate camera left/right/up/down");
	//glRasterPos2i(60, 360);
	//PrintString(GLUT_BITMAP_9_BY_15, "a and d - move camera left and right");
	//glRasterPos2i(60, 345);
	//PrintString(GLUT_BITMAP_9_BY_15, "w and s - move camera forward and backward");
	//glRasterPos2i(60, 330);
	//PrintString(GLUT_BITMAP_9_BY_15, "r and f - move camera up and down");
	//glRasterPos2i(60, 315);
	//PrintString(GLUT_BITMAP_9_BY_15, "PageUp and PageDown - move camera target up and down");
	//glRasterPos2i(60, 300);
	//PrintString(GLUT_BITMAP_9_BY_15, "+ and - - to select next/previous object");
	//glRasterPos2i(60, 285);
	//PrintString(GLUT_BITMAP_9_BY_15, "2, 3, 4, 5, 6, 8, 9 - to move selected object");
	//glRasterPos2i(60, 270);
	//PrintString(GLUT_BITMAP_9_BY_15, "l - reset load balancing procedure");
	//glRasterPos2i(60, 255);
	//PrintString(GLUT_BITMAP_9_BY_15, "k - toggle workload visualization");
	//glRasterPos2i(60, 240);
	//PrintString(GLUT_BITMAP_9_BY_15, "n, m - select previous/next OpenCL device");
	//glRasterPos2i(60, 225);
	//PrintString(GLUT_BITMAP_9_BY_15, "v, b - increase/decrease the worload of the selected OpenCL device");

	// computingUnits
	const std::vector< ComputingUnit*> computingUnits = rtConfig->GetComputingItem();
	double minPerf = computingUnits[0]->GetPerformance();
	double totalPerf = rtConfig->GetPerformanceIndex(0);
	double totalAmount = computingUnits[0]->GetWorkAmount();

	for (size_t i = 1; i < computingUnits.size(); ++i) {

		minPerf = std::min<double>(minPerf, computingUnits[i]->GetPerformance());
		totalPerf += rtConfig->GetPerformanceIndex(i);
		totalAmount += computingUnits[i]->GetWorkAmount();
	}

	glColor3f(1.0f, 0.5f, 0.f);
	int offset = 85;
	char buff[512];
	for (size_t i = 0; i < computingUnits.size(); ++i) {

		sprintf(buff, "[%s][Perf. Idx %.2f][Assigned Idx %.2f][Workload %.1f%%]", computingUnits[i]->GetDeviceName().c_str(),
				computingUnits[i]->GetPerformance() / minPerf,
			rtConfig->GetPerformanceIndex(i) / totalPerf,
				100.0 * computingUnits[i]->GetWorkAmount() / totalAmount);

		// Check if it is the selected device
		if (i == rtConfig->selectedDevice) {
				glColor3f(0.f, 0.f, 1.f);
				glRecti(10, offset - 5, 630, offset + 10);
				glColor3f(1.0f, 0.5f, 0.f);
		}

		glRasterPos2i(15, offset);
		PrintString(GLUT_BITMAP_9_BY_15, buff);

		offset += 16;
	}

	glRasterPos2i(12, offset);
	PrintString(GLUT_BITMAP_9_BY_15, "OpenCL Devices:");
}

static void PrintCaptions() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.f, 0.8f);
	glRecti(0, rtConfig->height - 15,
		rtConfig->width - 1, rtConfig->height - 1);
	glRecti(0, 0, rtConfig->width - 1, 20);
	glDisable(GL_BLEND);

	// Caption line 0
	if (rtConfig->IsProfiling()) {
		glColor3f(1.f, 0.f, 0.f);
		glRasterPos2i(4, 5);
		PrintString(GLUT_BITMAP_8_BY_13, "[Profiling]");
		glColor3f(1.f, 1.f, 1.f);
		glRasterPos2i(4 + glutBitmapLength(GLUT_BITMAP_8_BY_13, (unsigned char*)"[Profiling]"), 5);
		PrintString(GLUT_BITMAP_8_BY_13, rtConfig->captionBuffer);
	} else {
		glColor3f(1.f, 1.f, 1.f);
		glRasterPos2i(4, 5);
		PrintString(GLUT_BITMAP_8_BY_13, rtConfig->captionBuffer);
	}

	// Title
	glRasterPos2i(4, rtConfig->height - 10);
	PrintString(GLUT_BITMAP_8_BY_13, "OpenCL Ray Tracing Experiment");
}

static void displayFunc(void) {

	glRasterPos2i(0, 0);
	glDrawPixels(rtConfig->width, rtConfig->height, GL_RGBA, GL_UNSIGNED_BYTE, rtConfig->pixels);

	if (showWorkLoad) {
		const std::vector<ComputingUnit *> computingUnits = rtConfig->GetComputingItem();
		int start = 0;
		for (size_t i = 0; i < computingUnits.size(); ++i) {
			const int end = (computingUnits[i]->GetWorkOffset() + computingUnits[i]->GetWorkAmount()) / rtConfig->width;

			switch (i % 4) {
				case 0:
					glColor3f(1.f, 0.f, 0.f);
					break;
				case 1:
					glColor3f(0.f, 1.f, 0.f);
					break;
				case 2:
					glColor3f(0.f, 0.f, 1.f);
					break;
				case 3:
					glColor3f(1.f, 1.f, 0.f);
					break;
			}
			glRecti(0, start, 10, end);
			glBegin(GL_LINES);
			glVertex2i(0, start);
			glVertex2i(rtConfig->width, start);
			glVertex2i(0, end);
			glVertex2i(rtConfig->width, end);
			glEnd();


			glColor3f(1.f, 1.f, 1.f);
			glRasterPos2i(12, (start + end) / 2);
			PrintString(GLUT_BITMAP_8_BY_13, computingUnits[i]->GetDeviceName().c_str());
			start = end + 1;
		}
	}

	PrintCaptions();

	if (printHelp) {
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-0.5, 639.5, -0.5, 479.5, -1.0, 1.0);

		PrintHelpAndItems();

		glPopMatrix();
	}

	glutSwapBuffers();
}

void reshapeFunc(int newWidth, int newHeight) {
	rtConfig->width = newWidth;
	rtConfig->height = newHeight;

	glViewport(0, 0, newWidth, newHeight);
	glLoadIdentity();
	glOrtho(0.f, newWidth - 1.0f, 0.f, newHeight - 1.0f, -1.f, 1.f);

	rtConfig->ReInit(true);

	glutPostRedisplay();
}


void OnMouseMove(int x, int y)
{
	if (isMouseTracking)
	{
		//std::string tmp = " OnMouseMove: " + std::to_string(x) + " " + std::to_string(y);

		//std::cerr << tmp << " original: " 
		//	<< std::to_string(rtConfig->camera->orig.x) << " "
		//	<< std::to_string(rtConfig->camera->orig.y) << " "
		//	<< std::to_string(rtConfig->camera->target.x) << " "
		//	<< std::to_string(rtConfig->camera->target.y) << " "
		//	<<std::endl;


		float mouseDeltaX = (x - mousePos.first);
		float mouseDeltaY = (y - mousePos.second);

		mousePos.first = x;
		mousePos.second = y;


		//std::cerr << "pos: "
		//	<< std::to_string(mousePos.first) << " "
		//	<< std::to_string(mousePos.second) << " "
		//	<< std::endl;

		std::cerr << "delta: "
			<< std::to_string(mouseDeltaX) << " "
			<< std::to_string(mouseDeltaY) << " "
			<< std::endl;

		Vec dir = rtConfig->camera->target - rtConfig->camera->orig;
		dir.norm();

		rtConfig->camera->orig.x += (dir.x) * mouseDeltaY * 5;
		rtConfig->camera->orig.y += (dir.y) * mouseDeltaY * 5;
		rtConfig->camera->orig.z += (dir.z) * mouseDeltaY * 5;

		rtConfig->camera->target.x += (dir.x) * mouseDeltaY * 5;
		rtConfig->camera->target.y += (dir.y) * mouseDeltaY * 5;
		rtConfig->camera->target.z += (dir.z) * mouseDeltaY * 5;

		rtConfig->ReInit(false);


	}
}

void OnMouseButton(int btn, int state, int x, int y)
{
	if (btn == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			isMouseTracking = true;
			mousePos = std::make_pair(x, y);
		}
		else if (state == GLUT_UP && isMouseTracking)
		{
			isMouseTracking = true;
		}
	}
}

static void idleFunc(void) {
	UpdateRendering();

	glutPostRedisplay();
}

void InitGlut(int argc, char *argv[], unsigned int width, unsigned int height) {

	glutInitWindowSize(width, height);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInit(&argc, argv);

	glutCreateWindow("OpenCL Parallel Ray Tracing");
}

void RunGlut() {
	glutReshapeFunc(reshapeFunc);
//	glutKeyboardFunc(keyFunc);
//	glutSpecialFunc(specialFunc);
	glutDisplayFunc(displayFunc);
	glutMouseFunc(OnMouseButton);
	glutMotionFunc(OnMouseMove);

	glutIdleFunc(idleFunc);


	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, rtConfig->width, rtConfig->height);
	glLoadIdentity();
	glOrtho(0.f, rtConfig->width - 1.f, 0.f, rtConfig->height - 1.f, -1.f, 1.f);

	glutMainLoop();
}