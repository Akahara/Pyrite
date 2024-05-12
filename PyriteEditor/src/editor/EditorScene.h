#pragma once

#include <filesystem>

#include "scene/Scene.h"

namespace pye
{
	
class EditorScene : public pyr::Scene
{
public:
  explicit EditorScene(const std::filesystem::path& sceneFile);

  void update(double delta) override;
  void render() override;

private:
  void loadSceneFile(const std::filesystem::path& sceneFile);

private:
  std::filesystem::path m_sceneFile;
};

}
