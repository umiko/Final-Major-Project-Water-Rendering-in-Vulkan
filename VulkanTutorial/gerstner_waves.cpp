#include "gerstner_waves.hpp"


float Gerstner::get_phase_constant()
{
	return speed * 2 * PI / lambda;
}

float Gerstner::get_Q()
{
	return 1 / (get_w()*A);
}

float Gerstner::get_w() {
	return sqrtf(g * get_K());
}

float Gerstner::get_K() {
	return 2 * PI / lambda;
}

glm::vec3 Gerstner::get_displacement(glm::vec2 x0)
{
	float x = x0.x + (get_Q()*A*k.x*cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float y = x0.y + (get_Q()*A*k.y*cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float z = A*sinf(get_w()*glm::dot(k, x0)+get_phase_constant()*time);

	return glm::vec3(x,y,z);
}

Gerstner::Gerstner(glm::vec2 wave_direction, float amplitude, float wavelength, float speed)
{
	this->k = glm::normalize(wave_direction);
	this->A = amplitude;
	this->lambda = wavelength;
	this->speed = speed;
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
		glm::vec3 displacement = get_displacement(x0);
		new_displacement[i] = { current_displacement[i].displacement+displacement*step };
	}
	return new_displacement;
}
