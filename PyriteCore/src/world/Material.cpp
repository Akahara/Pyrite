#include "Material.h"

#include <filesystem>
#include "Mesh/Mesh.h"



pyr::Material::Material(const std::filesystem::path& shaderPath)
{
    m_shader = m_grr.loadEffect(shaderPath, InputLayout::MakeLayoutFromVertex<pyr::Mesh::mesh_vertex_t>());
}
