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
		static std::vector<Mesh> ImportMeshesFromFile(const fs::path& filePath)
		{
			Assimp::Importer importer;

			const aiScene* scene = importer.ReadFile(filePath.string().c_str(), aiProcess_Triangulate | aiProcess_PreTransformVertices);
			PYR_ASSERT(scene, "Could not load mesh ", filePath);
			std::vector<Mesh> outMeshes;
			ProcessNode(scene->mRootNode, scene, outMeshes);
			return outMeshes;
		}
	
		static std::vector<MaterialMetadata> FetchMaterialPaths(const fs::path& meshPath)
		{

			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(meshPath.string().c_str(), 0);

			if (!scene) throw 3; // error log here

			aiMaterial** Materials = scene->mMaterials;
			aiString* outputPath = new aiString;

			std::vector<MaterialMetadata> res;
			std::set<int> cachedIndices;

			res.resize(scene->mNumMaterials - 1);

			for (size_t meshId = 0; meshId < scene->mNumMeshes; ++meshId)
			{
				aiMesh* mesh = scene->mMeshes[meshId];
				unsigned int matId = mesh->mMaterialIndex;
				if (cachedIndices.contains(matId)) continue;

				cachedIndices.insert(matId);
				aiMaterial* currMeshMaterial = Materials[matId];

				res[matId].materialName = currMeshMaterial->GetName().C_Str();
				
				MaterialCoefs coefs;
				aiColor3D color;
				ai_real factor;

				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
					coefs.Ka = {color.r, color.g, color.b};
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color))
					coefs.Ks = {color.r, color.g, color.b};
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color))
					coefs.Ke = {color.r, color.g, color.b};
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_REFRACTI, factor))
					coefs.Ni = factor;
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_OPACITY, factor))
					coefs.d = factor;
				//if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_SHININESS, factor))
				//	coefs.Metallic = factor;
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_REFRACTI, factor))
					coefs.Roughness = factor / 1000.f;

				res[matId].coefs = coefs;
				for (auto [assimpType, pyrType] : std::vector<std::pair<aiTextureType, TextureType>>{
					{ aiTextureType_DIFFUSE, TextureType::ALBEDO },
					{ aiTextureType_NORMALS, TextureType::NORMAL},
					{ aiTextureType_METALNESS, TextureType::METALNESS},
					{ aiTextureType_SPECULAR, TextureType::SPECULAR},
					{ aiTextureType_AMBIENT, TextureType::AO},
					{ aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS},
					{ aiTextureType_DISPLACEMENT, TextureType::BUMP},
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
private:
			static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& outMeshes)
			{
				// process all the node's meshes (if any)
				for (unsigned int i = 0; i < node->mNumMeshes; i++)
				{
					aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
					outMeshes.push_back(ProcessMeshFromAssimp(mesh, scene));
				}
				// then do the same for each of its children
				for (unsigned int i = 0; i < node->mNumChildren; i++)
				{
					ProcessNode(node->mChildren[i], scene, outMeshes);
				}
			}

			static pyr::Mesh ProcessMeshFromAssimp(aiMesh* aimesh, const aiScene* scene)
			{
				std::vector<SubMesh> submeshes{};
				std::vector<Mesh::mesh_vertex_t> vertices;
				std::vector<Mesh::mesh_indice_t> indices;


				IndexBuffer::size_type startSubmeshIndex = static_cast<IndexBuffer::size_type>(indices.size());

				// Vertex processing
				for (size_t verticeId = 0; verticeId < aimesh->mNumVertices; verticeId++)
				{
					aiVector3D position = aimesh->mVertices[verticeId];
					aiVector3D normal = aimesh->HasNormals() ? aimesh->mNormals[verticeId] : aiVector3D{ 0, 0, 0 };
					aiVector3D uv = aimesh->HasTextureCoords(0) ? aimesh->mTextureCoords[0][verticeId] : aiVector3D{ 0, 0, 0 };

					Mesh::mesh_vertex_t computedVertex{};

					computedVertex.position = vec4{ position.x, position.y , -position.z , 1.f };
					computedVertex.normal = *reinterpret_cast<Vector3*>(&normal);
					computedVertex.normal.z *= -1;
					computedVertex.texCoords = vec2{ uv.x, uv.y };
					vertices.push_back(computedVertex);
				}

				// Index processing
				for (size_t faceId = 0; faceId < aimesh->mNumFaces; faceId++)
				{
					const aiFace& face = aimesh->mFaces[faceId];
					if (face.mNumIndices != 3) continue; // error logging here

					indices.push_back(face.mIndices[0] + startSubmeshIndex);
					indices.push_back(face.mIndices[1] + startSubmeshIndex);
					indices.push_back(face.mIndices[2] + startSubmeshIndex);
				}

				aiMaterial* currMeshMaterial = scene->mMaterials[aimesh->mMaterialIndex];

				submeshes.push_back(SubMesh{
					.startIndex = static_cast<UINT>(startSubmeshIndex),
					.endIndex = static_cast<UINT>(indices.size()),
					.materialIndex = static_cast<int>(aimesh->mMaterialIndex) ,
					.matName = currMeshMaterial->GetName().C_Str()
					});
			
				return Mesh{ vertices, indices, submeshes };
			}

	};
}
