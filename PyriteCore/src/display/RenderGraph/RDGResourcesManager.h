#pragma once

#include <set>

#include "NamedResources.h"
#include <unordered_map>

namespace pyr
{

	class RenderPass;

	class RenderGraphResourceManager
	{

		struct PassResources
		{
			std::unordered_map<std::string, NamedInput> incomingResources;
			std::unordered_map<std::string, NamedOutput> producedResources;

			std::set<std::string> requiredResources; // if a res is present here and not in the inputs, will throw error
		};

		std::unordered_map<RenderPass*, PassResources> m_passResources;

	public:

		/// Add a named input to a render pass if the source renderpass produces said resource.
		void linkResource(RenderPass* from, const char* resName, RenderPass* to);

		void addProduced(RenderPass* pass, const char* resName);
		void addRequirement(RenderPass* pass, const char* resName);

		bool checkResourcesValidity();

	private:

		friend class RenderGraph;
		void addNewPass(RenderPass* pass)
		{
			m_passResources[pass] = PassResources{};
		}

	};



}