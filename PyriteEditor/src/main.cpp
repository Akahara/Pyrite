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
#include "CubemapBuilderScene.h"
#include "TriangleScene.h"
#include "VoxelisationScene.h"
#include "MaterialScene.h"
#include "SponzaScene.h"
#include "ShadowScene.h"
#include "CornellBoxScene.h"
#include "utils/Debug.h"
#include "engine/Engine.h"
#include "engine/Device.h"
#include "scene/SceneManager.h"
#include "utils/StringUtils.h"
#include "editor/ShaderReloader.h"

#ifdef _CONSOLE
int main(int argc, char* argv[])
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
#ifndef PYR_ISDEBUG
  try
  {
#endif

    pyr::EngineSettings settings;
    settings.appTitle = "PyriteEditor";
    settings.bHasTitleBar = true;
    
    pye::ShaderAutoReloader shaderAutoReloader; // RAII singleton

    HINSTANCE handle = GetModuleHandle(NULL);

    pyr::Engine engine{ handle, std::move(settings) };
    pyr::SceneManager& scenes = pyr::SceneManager::getInstance();
    scenes.registerScene<pye::TriangleScene>("TriangleScene");
    scenes.registerScene<pye::EmptyEditorScene>("Empty");
    scenes.registerScene<pye::ForwardPassScene>("Forward pass");
    scenes.registerScene<pye::RayTracingDemoScene>("RayTracing demo");
    scenes.registerScene<pye::VoxelisationDemoScene>("Voxelisation demo");
    scenes.registerScene<pye::MaterialScene>("GGX Demo");
    scenes.registerScene<pye::CubemapBuilderScene>("IBL");
    scenes.registerScene<pye::CornellBoxScene>("CornellBox");
    scenes.registerScene<pye::ShadowScene>("ShadowScene");
    scenes.registerScene<pye::SponzaScene>("Sponza");

    // Load the scene that is passed on the command line by default
#ifdef _CONSOLE
    if (argc > 1)
        scenes.transitionToScene(argv[0]);
#else
    scenes.transitionToScene(pyr::widestring2string(lpCmdLine));
#endif
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
