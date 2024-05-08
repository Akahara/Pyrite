#include "scene_manager.h"

#include "utils/debug.h"

namespace pyr
{

SceneManager SceneManager::s_singleton{};

void SceneManager::setInitialScene(std::unique_ptr<Scene> &&scene)
{
  PYR_ASSERT(!m_activeScene, "A scene is already active");
  m_activeScene = std::move(scene);
}

void SceneManager::transitionToScene(SceneSupplier nextSceneSupplier)
{
  m_nextScene = std::move(nextSceneSupplier);
}

void SceneManager::dispose()
{
  m_activeScene.reset();
}

bool SceneManager::doSceneTransition()
{
  if (m_nextScene)
  {
    m_activeScene.reset();
    m_activeScene = m_nextScene();
    m_nextScene = {};
    return true;
  }
  return false;
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
