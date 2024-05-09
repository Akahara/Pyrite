#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

#include "Mesh.h"

namespace fs = std::filesystem;
using namespace DirectX::SimpleMath;

// todo : find a better place for this file
namespace pyr
{
	class MeshImporter
	{
	public:
		static Mesh ImportMeshFromFile(const fs::path& filePath, float importScale = 1.F)
		{
			Assimp::Importer importer;

			const aiScene* scene = importer.ReadFile(filePath.string().c_str(), aiProcess_Triangulate  );

			if (!scene)
				throw 3; // error log here

			// Todo : Once we have actual material support, we have to change this !

			std::vector<SubMesh> submeshes{SubMesh{.startIndex = 0}};

			std::vector<Mesh::mesh_vertex_t> vertices;
			std::vector<Mesh::mesh_indice_t> indices;

			for (size_t meshId = 0; meshId < scene->mNumMeshes; ++meshId)
			{
				aiMesh* mesh = scene->mMeshes[meshId];

				for (size_t verticeId = 0; verticeId < mesh->mNumVertices; verticeId++)
				{
					aiVector3D position = mesh->mVertices[verticeId];
					aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[verticeId] : aiVector3D{ 0, 0, 0 };
					aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][verticeId] : aiVector3D{ 0, 0, 0 };


					Mesh::mesh_vertex_t computedVertex{};

					computedVertex.position = *reinterpret_cast<Vector3*>(&position) * importScale;
					computedVertex.texCoords = Vector2{ uv.x, uv.y };
					vertices.push_back(computedVertex);
				}

				for (size_t faceId = 0; faceId < mesh->mNumFaces; faceId++)
				{
					const aiFace& face = mesh->mFaces[faceId];
					if (face.mNumIndices != 3) continue; // error logging here
					
					indices.push_back(face.mIndices[0]);
					indices.push_back(face.mIndices[1]);
					indices.push_back(face.mIndices[2]);
				}

				submeshes.push_back(SubMesh{ .startIndex = static_cast<IndexBuffer::size_type>(indices.size()) });
			}

			return Mesh{ vertices, indices, submeshes };
		}
	};
}
