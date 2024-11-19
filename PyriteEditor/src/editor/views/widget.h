#pragma once

#include "imgui.h"

#include <vector>
#include <memory>

static inline PYR_DEFINELOG(LogWidgets, VERBOSE);

namespace pye
{

	class Widget {

	public:

		virtual void display() {};
		bool bDisplayWidget = false;
	
	protected:

		static pyr::GraphicalResourceRegistry& getWidgetAssetRegistry()
		{
			static pyr::GraphicalResourceRegistry grr{};
			return grr;
		}
	};
}