#include "scene_manager.h"

namespace pyr
{

SceneManager SceneManager::s_singleton{};

void SceneManager::setActiveScene(std::unique_ptr<Scene> &&scene)
{
  m_activeScene = std::move(scene);
}

void SceneManager::disposeAllScenes()
{
  m_activeScene.reset();
}

void SceneManager::update(double delta) const
{
  m_activeScene->update(delta);
}

void SceneManager::render() const
{
  m_activeScene->render();
}

}
