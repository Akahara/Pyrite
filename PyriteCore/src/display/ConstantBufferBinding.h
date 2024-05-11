#pragma once

#include <memory>
#include "ConstantBuffer.h"

// goal : a shader has multiplice cb bindings that need to be filled ?

namespace pyr
{
	struct ConstantBufferBinding
	{
		enum BindFlag { 
			Ignorable, // Nothing happens if this binding is not resolved
			Warning, // Logs a warning if the cbuffer is not filled
			Error // throws an error
		};

		BindFlag Flag = BindFlag::Warning; // this might be useless
		
		std::string label;
		std::shared_ptr<BaseConstantBuffer> bufferRef;
	};
}