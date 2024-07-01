#include <algorithm>
#include <tchar.h>
#include <Windows.h>

#include "engine/Engine.h"
#include "engine/Device.h"
#include "scene/SceneManager.h"
#include "utils/Debug.h"
#include "utils/StringUtils.h"

#include "EditorScene.h"
#include "ForwardPassScene.h"
#include "RayTracingDemoScene.h"
#include "TriangleScene.h"
#include "VoxelisationScene.h"
#include "editor/ShaderReloader.h"

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifndef PYR_ISDEBUG
  try
  {
#endif
    pyr::EngineSettings settings;
    settings.appTitle = "PyriteEditor";
    settings.bHasTitleBar = true;
    
    pye::ShaderAutoReloader shaderAutoReloader; // RAII singleton

    pyr::Engine engine{ hInstance, std::move(settings) };
    pyr::SceneManager& scenes = pyr::SceneManager::getInstance();
    scenes.registerScene<pye::TriangleScene>("TriangleScene");
    scenes.registerScene<pye::EmptyEditorScene>("Empty");
    scenes.registerScene<pye::ForwardPassScene>("Forward pass");
    scenes.registerScene<pye::RayTracingDemoScene>("RayTracing demo");
    scenes.registerScene<pye::VoxelisationDemoScene>("Voxelisation demo");

    // Load the scene that is passed on the command line by default
    scenes.transitionToScene(pyr::widestring2string(lpCmdLine));
    engine.run();
    return 0;
#ifndef PYR_ISDEBUG
  }
  catch (const std::exception& e)
  {
    wchar_t message[512];
    mbstowcs_s(nullptr, message, e.what(), std::size(message) - 1);
    MessageBox(nullptr, message, L"Fatal error!", MB_ICONWARNING);
    return -1;
  }
#endif
}
