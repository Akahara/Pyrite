#include "SceneManager.h"

#include "imgui.h"
#include "utils/Debug.h"
#include "utils/ImGuiExt.h"

namespace pyr
{

SceneManager SceneManager::s_singleton{};

void SceneManager::transitionToScene(SceneSupplier nextSceneSupplier)
{
  if (m_activeScene == nullptr)
    m_activeScene = nextSceneSupplier();
  else
    m_nextScene = std::move(nextSceneSupplier);
  m_activeSceneName.resize(0);
}

bool SceneManager::transitionToScene(const std::string& sceneName)
{
  SceneSupplier initialSceneSupplier = getRegisteredScene(sceneName);
  if (initialSceneSupplier)
  {
    transitionToScene(std::move(initialSceneSupplier));
    m_activeSceneName = sceneName;
    return true;
  }
  return false;
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

void SceneManager::update(double delta)
{
  m_activeScene->update(delta);
}

void SceneManager::render()
{
  m_activeScene->render();

  if (m_knownScenes.size() > 1)
  {
    ImGui::Begin("Scenes");
    for (auto &[name, _] : m_knownScenes)
    {
      if (name == m_activeSceneName
        ? ImGui::ColoredButton(name.c_str(), ImVec4{ 0.191f, 0.581f, 0.224f, 1.f })
        : ImGui::Button(name.c_str()))
        transitionToScene(name);
    }
    ImGui::End();
  }
}

}
