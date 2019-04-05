#pragma once
#include <glm/gtc/type_aligned.hpp>
namespace brtr
{
    struct directional_light
    {
        glm::aligned_vec3 direction;
        glm::aligned_vec3 colour;
    };
} // namespace brtr
