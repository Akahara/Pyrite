#pragma once

#include <memory>

#include "display/shader.h"

namespace pye {

struct ShaderAutoReloaderImpl;

class ShaderAutoReloader {
public:
  ShaderAutoReloader();
  ~ShaderAutoReloader();

private:
  std::unique_ptr<ShaderAutoReloaderImpl> m_impl;
  pyr::ShaderManager::ShaderCreationHookHandle m_hookHandle;
};

}
