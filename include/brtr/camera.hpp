#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
        void move_forward(float amount)
        {
            m_position += m_look_at * amount;
        }
        void move_right(float amount)
        {
            m_position += glm::cross(m_up, m_look_at) * -amount;
        }
        glm::vec3 position() const
        {
            return m_position;
        }
        void rotate(float amount, glm::vec3 axis)
        {
            m_rotation *= glm::angleAxis(amount * 0.1f, axis * m_rotation);
            m_look_at = glm::vec3(0, 0, -1) * m_rotation;
        }

        void modify_pitch(float amount)
        {
            m_pitch += amount;
            glm::clamp(m_pitch, -89.0f, 89.0f);
        }
        void modify_yaw(float amount)
        {
            m_yaw += amount;
            if (m_yaw > 360.0f)
                m_yaw -= 360.0f;
            if (m_yaw < 0.0f)
                m_yaw += 360.0f;
        }

        void set_yaw(float value)
        {
            m_yaw = value;
        }
        camera_gpu gpu();

    private:
        glm::vec3 m_position;
        glm::vec3 m_look_at;
        glm::vec3 m_up;
        glm::quat m_rotation;
        float m_vertical_fov;
        float m_aspect;
        float m_pitch;
        float m_yaw;
    };
} // namespace brtr
