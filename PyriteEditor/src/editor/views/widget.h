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
	
	protected:

		static pyr::GraphicalResourceRegistry& getWidgetAssetRegistry()
		{
			static pyr::GraphicalResourceRegistry grr{};
			return grr;
		}
	};

	struct WidgetsContainer
	{
		std::vector<Widget*> widgets;
		void Render()
		{
			for (Widget* widget : widgets)
				widget->display();
		}
	};

}