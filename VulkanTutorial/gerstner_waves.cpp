#include "gerstner_waves.hpp"


float Gerstner::get_phase_constant()
{
	return speed * 2 / lambda;
}

//"Choppiness" of the wave
float Gerstner::get_Q()
{
	return 0.4f;
}

//Tessendorf calls it K, everyone else w, its a bit confusing
float Gerstner::get_w() {
	//return sqrtf((g * get_K()));
	return get_K();
}

float Gerstner::get_K() {
	return 2 * PI / lambda;
}

glm::vec3 Gerstner::get_displacement(glm::vec2 x0, glm::vec3 displacement)
{
	float x = displacement.x + (get_Q()*A*k.x*cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float y = displacement.y + (get_Q()*A*k.y*cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float z = displacement.z + A*sinf(get_w()*glm::dot(k, x0)+get_phase_constant()*time);

	/*float x = (get_Q()*A*k.x * cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float y = (get_Q()*A*k.y * cosf(get_w()*glm::dot(k, x0) + get_phase_constant()*time));
	float z = A * sinf(get_w()*glm::dot(k, x0) + get_phase_constant()*time);*/

	/*float x = ((k.x/get_K())*A*k.x * cosf(get_w()*glm::dot(k, x0) -get_w() * time + get_phase_constant()));
	float y = ((k.x / get_K())*A*k.y * cosf(get_w()*glm::dot(k, x0) - get_w() * time + get_phase_constant()));
	float z = A * sinf(get_w()*glm::dot(k, x0) + get_phase_constant()*time);*/

	return glm::vec3(x, y, z);

	//return displacement;
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
		//get undisturbed vertex position 
		x0 = glm::vec2(i%resolution, floorf(i / resolution));
		glm::vec3 displacement = get_displacement(x0, current_displacement[i].displacement);
		//new_displacement[i] = { current_displacement[i].displacement + displacement * step };
		new_displacement[i] = { displacement };
	}
	return new_displacement;
}
