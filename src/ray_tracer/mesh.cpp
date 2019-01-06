#include "brtr/mesh.hpp"

namespace brtr
{
	mesh::mesh()
		:m_material()
	{
	}

	mesh::mesh(const std::vector<vertex>& vertices, const std::vector<int> indices, const mesh_material& material)
		:m_vertices(vertices), m_indices(indices), m_material(material)
	{

	}

	const std::vector<vertex>& mesh::vertices() const 
	{
		return m_vertices;
	}

	const mesh_material& mesh::material() const
	{
		return m_material;
	}

	const std::vector<int>& mesh::indices() const
	{
		return m_indices;
	}
}
