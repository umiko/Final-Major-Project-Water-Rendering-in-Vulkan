#include "gerstner_waves.hpp"


float Gerstner::get_w() {
	return sqrtf(g * get_K());
}

float Gerstner::get_K() {
	return 2 * PI / lambda;
}

float Gerstner::get_y(glm::vec2 x0) {
	//TODO: check if it really is the length
	return A * cosf(k.x * x0.x - get_w()*time);
}

glm::vec2 Gerstner::get_x(glm::vec2 x0) {
	return x0 - (k / get_K())*A*sinf(k.length() * x0.length() - get_w()*time);
}

Gerstner::Gerstner(glm::vec2 wave_direction, float amplitude, float wavelength)
{
	this->k = wave_direction;
	this->A = amplitude;
	this->lambda = wavelength;
}

//Applies this wave on top of a wavemap
std::vector<Displacement> Gerstner::apply_wave(std::vector<Displacement> current_displacement, uint32_t resolution, float tilesize, float time) {
	this->time = time;
	uint32_t size = resolution * resolution;
	float step = tilesize / resolution;
	
	std::vector<Displacement> new_displacement = { {} };
	new_displacement.resize(size);

	glm::vec2 x0;

	for (int i = 0; i < size; i++) {
		x0 = glm::vec2(i%resolution, floorf(i / resolution));
		new_displacement[i] = { glm::vec3(get_x(x0)*step, get_y(x0)*step)+current_displacement[i].displacement };
	}
	return new_displacement;
}
