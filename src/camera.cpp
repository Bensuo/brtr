#include "brtr/camera.hpp"
#include <glm/glm.hpp>

namespace brtr
{
	camera::camera()
	{
	}

	void camera::set_fov(float fov, float aspect_ratio)
	{
		m_vertical_fov = fov;
		m_aspect = aspect_ratio;
	}

	void camera::set_position(glm::vec3 position)
	{
		m_position = position;
	}

	camera_gpu camera::gpu()
	{
		camera_gpu cam;
		cam.position = m_position;

		float theta = glm::radians(m_vertical_fov);
		float half_height = tan(theta / 2);
		float half_width = m_aspect * half_height;

		glm::vec3 w = glm::normalize(m_position - m_look_at);
		glm::vec3 u = glm::normalize(glm::cross(m_up, w));
		glm::vec3 v = glm::cross(w, u);

		cam.lower_left = m_position - half_width * u - half_height * v - w;
		cam.horizontal = 2 * half_width * u;
		cam.vertical = 2 * half_height * v;

		return cam;
	}
}
