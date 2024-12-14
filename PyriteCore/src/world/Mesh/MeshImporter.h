#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <filesystem>

#include "RawMeshData.h"
#include "../Material.h"
#include "Model.h"
#include <set>
#include <array>

namespace fs = std::filesystem;
using namespace DirectX::SimpleMath;

// todo : find a better place for this file
namespace pyr
{
	class MeshImporter
	{
	public:
		static std::vector<std::shared_ptr<pyr::Model>> ImportMeshesFromFile(const fs::path& filePath, bool bFlipUVs = false)
		{
			Assimp::Importer importer;

			bool bExists = std::filesystem::exists(filePath);
			if (!bExists) return {};

			const aiScene* scene = importer.ReadFile(filePath.string().c_str(), aiProcess_Triangulate | aiProcess_PreTransformVertices | (bFlipUVs ? aiProcess_FlipUVs : 0) | aiProcess_FlipWindingOrder);
			PYR_ASSERT(scene, "Could not load mesh ", filePath);
			std::vector<std::shared_ptr<pyr::RawMeshData>> outMeshes;
			Model::SubmeshesMaterialTable defaultMaterials;
			defaultMaterials.resize(scene->mNumMaterials);
			ProcessNode(scene->mRootNode, scene, outMeshes, defaultMaterials);

			std::vector<std::shared_ptr<pyr::Model>> outModels;
			for (auto& meshData : outMeshes)
			{
				auto Model = std::make_shared<pyr::Model>(meshData, defaultMaterials);
				if (!Model) continue;
				outModels.emplace_back(Model);
			}
			return outModels;
		}
private:
			static void ProcessNode(
						aiNode* node, const aiScene* scene, 
						std::vector<std::shared_ptr<pyr::RawMeshData>>& outMeshes, 
						Model::SubmeshesMaterialTable& outDefaultMaterials
						)
			{
				// process all the node's meshes (if any)
				for (unsigned int i = 0; i < node->mNumMeshes; i++)
				{
					aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
					outMeshes.push_back(ProcessMeshFromAssimp(mesh, scene));

					if (outDefaultMaterials[mesh->mMaterialIndex] == nullptr)
					{
						MaterialBank::mat_id_t matGlobalId = CreateMaterialFromMesh(mesh, scene);
						const std::shared_ptr<Material>& computedDefaultMaterial = MaterialBank::GetMaterialReference(matGlobalId);
						outDefaultMaterials[mesh->mMaterialIndex] = computedDefaultMaterial;
					}

				}
				// then do the same for each of its children
				for (unsigned int i = 0; i < node->mNumChildren; i++)
				{
					ProcessNode(node->mChildren[i], scene, outMeshes, outDefaultMaterials);
				}
			}
			
			// This should actually process submeshes
			static std::shared_ptr<pyr::RawMeshData> ProcessMeshFromAssimp(aiMesh* aimesh, const aiScene* scene)
			{
				std::vector<SubMesh> submeshes{};
				std::vector<RawMeshData::mesh_vertex_t> vertices;
				std::vector<RawMeshData::mesh_indice_t> indices;

				IndexBuffer::size_type startSubmeshIndex = static_cast<IndexBuffer::size_type>(indices.size());

				// Vertex processing
				for (size_t verticeId = 0; verticeId < aimesh->mNumVertices; verticeId++)
				{
					aiVector3D position = aimesh->mVertices[verticeId];
					aiVector3D normal = aimesh->HasNormals() ? aimesh->mNormals[verticeId] : aiVector3D{ 0, 0, 0 };
					aiVector3D uv = aimesh->HasTextureCoords(0) ? aimesh->mTextureCoords[0][verticeId] : aiVector3D{ 0, 0, 0 };

					RawMeshData::mesh_vertex_t computedVertex{};

					computedVertex.position = vec4{ position.x, position.y , position.z , 1.f };
					computedVertex.normal = *reinterpret_cast<Vector3*>(&normal);
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
				submeshes.push_back(pyr::SubMesh{
					.startIndex = static_cast<UINT>(startSubmeshIndex),
					.endIndex = static_cast<UINT>(indices.size()),
					.materialIndex = static_cast<size_t>(aimesh->mMaterialIndex) ,
					.matName = currMeshMaterial->GetName().C_Str()
					});
			
				return std::make_shared<RawMeshData>(vertices, indices, submeshes);
			}

			static pyr::MaterialBank::mat_id_t CreateMaterialFromMesh(aiMesh* aimesh, const aiScene* scene)
			{
			
				aiMaterial* currMeshMaterial = scene->mMaterials[aimesh->mMaterialIndex];

				aiString* outputPath = new aiString;

				std::string materialName = currMeshMaterial->GetName().C_Str();
				MaterialTexturePathsCollection paths;
				MaterialRenderingCoefficients coefs;

				aiColor3D color; ai_real factor;
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
					coefs.Ka = { color.r, color.g, color.b };
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color))
					coefs.Ks = { color.r, color.g, color.b };
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color))
					coefs.Ke = { color.r, color.g, color.b };
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_REFRACTI, factor))
					coefs.Ni = factor;
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_OPACITY, factor))
					coefs.d = factor;
				//if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_SHININESS, factor))
				//	coefs.Metallic = factor;
				if (AI_SUCCESS == currMeshMaterial->Get(AI_MATKEY_REFRACTI, factor))
					coefs.Roughness = factor / 1000.f;

				for (auto& [assimpType, pyrType] : std::array< std::pair<aiTextureType, TextureType>, 8>{
					{{ aiTextureType_DIFFUSE, TextureType::ALBEDO },
						{ aiTextureType_NORMALS, TextureType::NORMAL },
						{ aiTextureType_METALNESS, TextureType::METALNESS },
						{ aiTextureType_SPECULAR, TextureType::SPECULAR },
						{ aiTextureType_DIFFUSE_ROUGHNESS, TextureType::AO },
						{ aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS },
						{ aiTextureType_DISPLACEMENT, TextureType::BUMP },
						{ aiTextureType_HEIGHT, TextureType::HEIGHT },
					}})
				{

					if (currMeshMaterial->GetTextureCount(assimpType) >= 1)
					{
						aiReturn ref = currMeshMaterial->GetTexture(assimpType, 0, outputPath);
						paths[pyrType] = "res/" + std::string(outputPath->C_Str());
					}
				}
				delete outputPath;
				std::shared_ptr<Material> toRegister = Material::MakeRegisteredMaterial( paths, coefs, pyr::MaterialBank::GetDefaultGGXShader(), materialName);
				return pyr::MaterialBank::GetMaterialGlobalId(materialName);
			}

	};
}
