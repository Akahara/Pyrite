#pragma once

#include <filesystem>

#include "scene/Scene.h"
#include "world/Actor.h"

namespace pye
{
	
class EditorScene : public pyr::Scene
{
public:
  explicit EditorScene(std::filesystem::path sceneFile);

  void update(double delta) override;
  void render() override;

private:
  void loadSceneFile(const std::filesystem::path& sceneFile);
  void writeSceneFile(const std::filesystem::path& sceneFile);

private:
  std::filesystem::path m_sceneFile;

  std::vector<std::unique_ptr<pyr::Actor>> m_actors;
};

}
