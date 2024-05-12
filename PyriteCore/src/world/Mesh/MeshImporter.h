#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <filesystem>

#include "Mesh.h"
#include "../Material.h"
#include <set>

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

			const aiScene* scene = importer.ReadFile(filePath.string().c_str(), aiProcess_Triangulate);

			PYR_ASSERT(scene, "Could not load mesh ", filePath);

			// Todo : Once we have actual material support, we have to change this !

			std::vector<SubMesh> submeshes{ SubMesh{} };

			std::vector<Mesh::mesh_vertex_t> vertices;
			std::vector<Mesh::mesh_indice_t> indices;

			aiMaterial** Materials = scene->mMaterials;

			for (size_t meshId = 0; meshId < scene->mNumMeshes; ++meshId)
			{
				aiMesh* mesh = scene->mMeshes[meshId];
				aiMaterial* currMeshMaterial = Materials[mesh->mMaterialIndex];
				aiString* outputpath = new aiString;

				if (currMeshMaterial->GetTextureCount(aiTextureType_DIFFUSE) >= 0)
					currMeshMaterial->GetTexture(aiTextureType_DIFFUSE, 0, outputpath);

				if (currMeshMaterial->GetTextureCount(aiTextureType_BASE_COLOR) >= 0)
					currMeshMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, outputpath);

				for (size_t verticeId = 0; verticeId < mesh->mNumVertices; verticeId++)
				{
					aiVector3D position = mesh->mVertices[verticeId];
					aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[verticeId] : aiVector3D{ 0, 0, 0 };
					aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][verticeId] : aiVector3D{ 0, 0, 0 };

					Mesh::mesh_vertex_t computedVertex{};

					computedVertex.position = vec4{ position.x * importScale, position.y * importScale, -position.z * importScale, 1.f };
					computedVertex.normal = *reinterpret_cast<Vector3*>(&normal);
					computedVertex.texCoords = vec2{ uv.x, uv.y };
					vertices.push_back(computedVertex);
				}

				IndexBuffer::size_type curr = static_cast<IndexBuffer::size_type>(indices.size());
				for (size_t faceId = 0; faceId < mesh->mNumFaces; faceId++)
				{
					const aiFace& face = mesh->mFaces[faceId];
					if (face.mNumIndices != 3) continue; // error logging here
					
					indices.push_back(curr + face.mIndices[0]);
					indices.push_back(curr + face.mIndices[1]);
					indices.push_back(curr + face.mIndices[2]);
				}

				IndexBuffer::size_type currentIndexStart = static_cast<IndexBuffer::size_type>(indices.size());
				submeshes.push_back(SubMesh{ 
					.startIndex = static_cast<UINT>(indices.size()),
					.materialIndex = static_cast<int>(mesh->mMaterialIndex),
					.matName = currMeshMaterial->GetName().C_Str()
					});


			}

			return Mesh{ vertices, indices, submeshes };
		}
	
		static std::vector<MaterialMetadata> FetchMaterialPaths(const fs::path& meshPath)
		{

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(meshPath.string().c_str(), aiProcess_Triangulate);


			if (!scene) throw 3; // error log here

			aiMaterial** Materials = scene->mMaterials;
			aiString* outputPath = new aiString;

			std::vector<MaterialMetadata> res;
			std::set<int> cachedIndices;

			res.resize(scene->mNumMaterials + 1);

			for (size_t meshId = 0; meshId < scene->mNumMeshes; ++meshId)
			{
				aiMesh* mesh = scene->mMeshes[meshId];
				unsigned int matId = mesh->mMaterialIndex;
				if (cachedIndices.contains(matId)) continue;

				cachedIndices.insert(matId);
				aiMaterial* currMeshMaterial = Materials[matId];
				res[matId].materialName = currMeshMaterial->GetName().C_Str();

				for (auto [assimpType, pyrType] : std::vector<std::pair<aiTextureType, TextureType>>{
					{ aiTextureType_DIFFUSE, TextureType::ALBEDO },
					{ aiTextureType_DISPLACEMENT, TextureType::NORMAL},
					{ aiTextureType_METALNESS, TextureType::METALNESS},
					{ aiTextureType_SPECULAR, TextureType::SPECULAR},
					{ aiTextureType_AMBIENT_OCCLUSION, TextureType::AO},
					{ aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS},
					//{ aiTextureType_, TextureType::BUMP},
					{ aiTextureType_HEIGHT, TextureType::HEIGHT},
					})
				{

					if (currMeshMaterial->GetTextureCount(assimpType) >= 1)
					{
						aiReturn ref = currMeshMaterial->GetTexture(assimpType, 0, outputPath);
						res[matId].paths[pyrType] = outputPath->C_Str();
					}
				}
			}

			delete outputPath;
			return res;
		}

	};
}
