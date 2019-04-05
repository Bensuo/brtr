#include "brtr/camera.hpp"
namespace brtr
{
    camera::camera()
        : m_position(),
          m_look_at(),
          m_up(),
          m_rotation(),
          m_vertical_fov(0),
          m_aspect(0),
          m_pitch(0),
          m_yaw(0)
    {
        m_rotation = glm::identity<glm::quat>();
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
        m_rotation =
            glm::quat(glm::vec3(-glm::radians(m_pitch), -glm::radians(m_yaw), 0));
        m_look_at = m_rotation * glm::vec3(0, 0, -1);

        camera_gpu cam;
        cam.position = m_position;

        float theta = glm::radians(m_vertical_fov);
        float half_height = tan(theta / 2);
        float half_width = m_aspect * half_height;

        glm::vec3 w = glm::normalize(m_position - (m_position + m_look_at));
        glm::vec3 u = glm::normalize(glm::cross(m_up, w));
        glm::vec3 v = glm::cross(w, u);

        cam.lower_left = m_position - half_width * u - half_height * v - w;
        cam.horizontal = 2 * half_width * u;
        cam.vertical = 2 * half_height * v;

        return cam;
    }
} // namespace brtr
