#pragma once
#include <glm/gtc/type_aligned.hpp>
namespace brtr
{
    struct point_light
    {
        glm::aligned_vec3 position;
        glm::aligned_vec3 colour;
        float radius;
    };
} // namespace brtr
