#pragma once

#include <vector>
#include <cmath>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include "displacement.hpp"

class Gerstner {
	//variables are named like they are in the formulas in Tessendorfs

private:
	glm::vec2 k; //direction of wave
	float A;	 //Amplitude

	float lambda; //wavelength
	float time; //time

	//constant values needed at some point
	const float g = 9.81; //gravity
	const float PI = 3.14;

	float speed;
	
	float get_phase_constant();

	float get_Q();
	//frequency of the waves
	//w^2(k) = g * K 
	float get_w();
	//magnitude
	//K=2PI/wavelength
	float get_K();

	glm::vec3 get_displacement(glm::vec2 x0, glm::vec3 displacement);

public:
	Gerstner(glm::vec2 wave_direction, float amplitude, float wavelength, float speed);

	//Applies this wave on top of a wavemap
	std::vector<Displacement> apply_wave(std::vector<Displacement> current_displacement, uint32_t resolution, float tilesize, float time);
	
};