#pragma once

#include <vector>
#include <cmath>

#include <glm\vec2.hpp>
#include <glm\vec3.hpp>

class Gerstner {

	//constant values needed at some point
	const float g = 9.81; //gravity
	const float PI = 3.14;

	//variables are named like they are in the formulas in Tessendorfs
	glm::vec2 x0;//point on the surface
	glm::vec2 k; //direction of wave
	float A;	 //Amplitude
	
	float lambda; //wavelength
	float time; //time
	
	//frequency of the waves
	//w^2(k) = g * K 
	float get_w();
	//magnitude
	//K=2PI/wavelength
	float get_K();
	//height
	//y=A*cos(k*x0-w*t)
	float get_y();
	//displacement
	//x=x0-(k/K)*A*sin(k*x0-w*t)
	glm::vec2 get_x();
	//wave length
	//lambda = 2PI/K



public:
	//Applies this wave on top of a wavemap
	void apply_wave(std::vector<glm::vec3> &current_waves, float time);
	
};