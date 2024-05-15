#include "RDGResourcesManager.h"

#include "RenderPass.h"

#define ENSURE_IS_IN_GRAPH(pass) if (!m_passResources.contains(pass)) throw Errors::PassNotInManager{};

namespace pyr
{


	struct Errors {
		struct PassDoesNotProduceResource {};
		struct PassNotInManager {};
		struct ResourceGraphNotValid {};
	};


	void RenderGraphResourceManager::linkResource(RenderPass* from, const char* resName, RenderPass* to)
	{
		ENSURE_IS_IN_GRAPH(from)
		ENSURE_IS_IN_GRAPH(to)

		std::optional<NamedOutput> resource = from->getOutputResource(resName);
		if (resource.has_value()) {
			to->addNamedInput(resource.value());
			m_passResources[to].incomingResources[resName] = resource.value();
		}

		else throw Errors::PassDoesNotProduceResource{} ;
	}

	void RenderGraphResourceManager::addProduced(RenderPass* pass, const char* resName)
	{
		ENSURE_IS_IN_GRAPH(pass)

		std::optional<NamedOutput> resource = pass->getOutputResource(resName);
		if (resource.has_value()) m_passResources[pass].producedResources[resName] = resource.value();

		else throw Errors::PassDoesNotProduceResource{};

	}

	void RenderGraphResourceManager::addRequirement(RenderPass* pass, const char* resName)
	{
		ENSURE_IS_IN_GRAPH(pass)

		m_passResources[pass].requiredResources.insert(resName);
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