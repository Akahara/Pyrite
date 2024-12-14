#include "RDGResourcesManager.h"

#include "RenderGraph.h"
#include "RenderPass.h"
#include "utils/debug.h"
#include <optional>

#define ASSERT_IS_IN_GRAPH(pass) PYR_ASSERT(m_passResources.contains(pass));

namespace pyr
{


	struct Errors {
		struct PassDoesNotProduceResource {};
		struct PassNotInManager {};
		struct ResourceGraphNotValid {};
	};


	void RenderGraphResourceManager::linkResource(RenderPass* from, const char* resName, RenderPass* to)
	{
		ASSERT_IS_IN_GRAPH(from);
		if (!PYR_ENSURE(m_passResources.contains(to)))
		{
			PYR_LOGF(LogRenderGraph, WARN, "Trying to link resource  \"{}\" to pass {} that is not in graph !", resName, to->displayName);
		}
		
		std::optional<NamedOutput> resource = from->getOutputResource(resName);
		if (resource.has_value()) {
			m_passResources[to].incomingResources[resName] = resource.value();
		}

		else throw Errors::PassDoesNotProduceResource{} ;
	}

	void RenderGraphResourceManager::linkResource(const char* resName, RenderPass* to)
	{
		ASSERT_IS_IN_GRAPH(to);

		for (auto& [pass, passResources] : m_passResources)
		{
			if (passResources.producedResources.contains(resName))
			{
				NamedOutput& output = passResources.producedResources[resName];
				m_passResources[to].incomingResources[resName] = output;
				return;
			}
		}

		PYR_ASSERT(false, "Trying to link a resource that no renderpass produces in the graph... very sad");
	}

	void RenderGraphResourceManager::addProduced(RenderPass* pass, const char* resName)
	{
		ASSERT_IS_IN_GRAPH(pass);

		std::optional<NamedOutput> resource = pass->getOutputResource(resName);
		if (resource.has_value()) m_passResources[pass].producedResources[resName] = resource.value();

		else throw Errors::PassDoesNotProduceResource{};

	}

	void RenderGraphResourceManager::addRequirement(RenderPass* pass, const char* resName)
	{
		ASSERT_IS_IN_GRAPH(pass);

		m_passResources[pass].requiredResources.insert(resName);
	}

	NamedResource::resource_t RenderGraphResourceManager::fetchResource(const char* resName)
	{
		for (auto& [pass, res] : m_passResources)
		{
			if (res.producedResources.contains(resName))
			{
				return res.producedResources.at(resName).res;
			}
		}

		PYR_ASSERT(false, "No render pass has been found to produce this resource. Did you make a typo, or did you forget to link them in the scene ?");
		return NamedResource::resource_t();
	}

	std::optional<NamedResource::resource_t> RenderGraphResourceManager::fetchOptionalResource(const char* resName)
	{
		for (auto& [pass, res] : m_passResources)
		{
			if (res.producedResources.contains(resName))
			{
				return res.producedResources.at(resName).res;
			}
		}

		return std::nullopt;
	}

	// throws error
	bool RenderGraphResourceManager::checkResourcesValidity()
	{
		for (auto& [pass, res] : m_passResources)
		{
			for (const std::string& resName : res.requiredResources)
			{
				// If a requirement is not met (= not in the inputs list)
				if (!res.incomingResources.contains(resName))
				{
#ifdef _DEBUG
					throw Errors::ResourceGraphNotValid{};
#endif 
					return false;
				}
			}
		}
		return true;
	}




}