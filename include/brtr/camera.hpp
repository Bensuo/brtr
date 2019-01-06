#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>

namespace brtr
{
	struct camera_gpu
	{
		glm::aligned_vec3 position;
		glm::aligned_vec3 lower_left;
		glm::aligned_vec3 horizontal;
		glm::aligned_vec3 vertical;
	};
	class camera
	{
	public:
		camera();
		void set_fov(float fov, float aspect_ratio);
		void set_position(glm::vec3 position);
		void set_look_at(glm::vec3 look_at)
		{
			m_look_at = look_at;
		}
		void set_up(glm::vec3 up)
		{
			m_up = up;
		}
		glm::vec3 position()
		{
			return m_position;
		}
		camera_gpu gpu();
	private:
		glm::vec3 m_position;
		glm::vec3 m_look_at;
		glm::vec3 m_up;
		float m_vertical_fov;
		float m_aspect;
	};
}
