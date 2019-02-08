#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>
#include <vector>

namespace brtr
{
    struct vertex
    {
        glm::aligned_vec3 position;
        glm::aligned_vec3 normal;
    };

    struct mesh_material
    {
        glm::aligned_vec3 diffuse;
        glm::aligned_vec3 specular;
        glm::aligned_vec3 emissive;
    };

    class mesh
    {
    public:
        mesh();
        mesh(const std::vector<vertex>& vertices, const std::vector<int> indices, const mesh_material& material);
        const std::vector<vertex>& vertices() const;
        const mesh_material& material() const;
        const std::vector<int>& indices() const;

    private:
        std::vector<vertex> m_vertices;
        std::vector<int> m_indices;
        mesh_material m_material;
    };
} // namespace brtr
