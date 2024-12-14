#pragma once

#include <set>
#include <optional>

#include "NamedResources.h"
#include <unordered_map>

namespace pyr
{
	// TODO : order passes correctly (current order is how they are push in the render graph...)
	class RenderPass;

	class RenderGraphResourceManager
	{
	public:
		struct PassResources
		{
			std::unordered_map<std::string, NamedInput> incomingResources;
			std::unordered_map<std::string, NamedOutput> producedResources;

			std::set<std::string> requiredResources; // if a res is present here and not in the inputs, will throw error
		};

	private:
		std::unordered_map<RenderPass*, PassResources> m_passResources;

	public:

		/// Add a named input to a render pass if the source renderpass produces said resource.
		void linkResource(RenderPass* from, const char* resName, RenderPass* to);

		// Will try to fetch the resource somewhere, not sure if above version is required
		// This is mainly used when you don't know what passes are in the render graph but need something (like depth buffer)
		// I do believe we should have types and constant for resource naming instead of strings
		void linkResource(const char* resName, RenderPass* to);

		void addProduced(RenderPass* pass, const char* resName);
		void addRequirement(RenderPass* pass, const char* resName);

		// -- Returns the first produced resource that matches the name.
		NamedResource::resource_t fetchResource(const char* resName);

		std::optional<NamedResource::resource_t> fetchOptionalResource(const char* resName);

		bool checkResourcesValidity();

		const std::unordered_map<RenderPass*, PassResources>& GetAllResources() const { return m_passResources; }

	private:

		friend class RenderGraph;
		void addNewPass(RenderPass* pass)
		{
			m_passResources[pass] = PassResources{};
		}

	};



}