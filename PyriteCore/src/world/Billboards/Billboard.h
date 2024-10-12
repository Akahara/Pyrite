#pragma once

#include "utils/math.h"
#include "world/Transform.h"
#include "world/Actor.h"
#include "display/vertex.h"
#include "display/VertexBuffer.h"
#include "display/IndexBuffer.h"
#include "display/GraphicalResource.h"

#include <type_traits>

static inline PYR_DEFINELOG(LogBillboards, VERBOSE);

namespace pyr
{
	struct Billboard : public Actor
	{
	public:

		using billboard_vertex_t = pyr::GenericVertex<INSTANCE_TRANSFORM, INSTANCE_UV, INSTANCE_TEXID, INSTANCE_DATA>;

		// Will give an ID 
		enum BillboardType
		{
			HUD,
			UNLIT,
			LIT,
		};

		BillboardType type;
		Transform transform;
		vec4 instanceUVs = { 0,0,1,1 };
		const pyr::Texture* texture = &pyr::Texture::getDefaultTextureSet().WhitePixel;

		friend class BillboardManager;

	};

	class BillboardManager
	{
	public:

		struct BillboardsRenderData
		{
			pyr::VertexBuffer instanceBuffer;
			std::unordered_map<const pyr::Texture*, int> textures; // < won't support more than 16, i don't care
		};
																													// offset, scale
		static BillboardsRenderData makeContext(const std::vector<const pyr::Billboard*>& sceneBillboards)
		{
			if (sceneBillboards.empty()) return {};

			BillboardsRenderData res;
			std::vector<Billboard::billboard_vertex_t> instanceVertices;

			for (const pyr::Billboard* billboard : sceneBillboards)
			{
				// -- Register texture and get id
				if (!PYR_ENSURE(res.textures.size() < 16))
				{
					PYR_LOGF(LogBillboards, WARN, "Trying to add a billboard, but more than 16 textures have been registered. Not implementing that soon. Too bad !");
					break; // or continue ?
				}

				if (!res.textures.contains(billboard->texture))
				{
					res.textures[billboard->texture] = res.textures.size();
				} 

				Billboard::billboard_vertex_t vertex;
				vertex.instanceTexId		= res.textures[billboard->texture];
				vertex.instanceTransform	= billboard->transform.getWorldMatrix();
				vertex.instanceUvs			= billboard->instanceUVs;
				vertex.data					= (float)billboard->type;
				instanceVertices.push_back(vertex);

			}

			res.instanceBuffer = pyr::VertexBuffer{ instanceVertices };
			return res;
		}


	private:

		static BillboardManager& Get() {
			static BillboardManager manager;
			return manager;
		}

		BillboardManager() = default;
	};

}