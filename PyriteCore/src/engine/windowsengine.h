#pragma once

#include "d3ddevice.h"
#include "display/graphical_resource.h"
#include "utils/clock.h"

namespace pyr
{
class Effect;

struct EngineSettings {
  std::string appTitle;
  bool bHasTitleBar = true;
};

class Engine
{
public:
  Engine(HINSTANCE hInstance, EngineSettings settings);
  ~Engine();

  void run();

  static const EngineSettings& getEngineSettings() { return s_engineSingleton->m_settings; }
  // Can be used during engine initialization to test if the device was already initialized, not necessary after the engine's constructor completed
  static bool hasDevice() { return s_engineSingleton->m_device != nullptr; }
  static Device &device() { return *s_engineSingleton->m_device; }
  static ID3D11Device &d3ddevice() { return s_engineSingleton->m_device->getDevice(); }
  static ID3D11DeviceContext &d3dcontext() { return s_engineSingleton->m_device->getImmediateContext(); }

private:
  void initAppInstance();
  void showWindow();
  void runFrame(float deltaTime);
  void postFrame();
  bool pollEvents();

private:
  EngineSettings m_settings;

  HINSTANCE m_appInstance;
  HACCEL    m_accelTable = nullptr;
  HWND      m_mainWindowHandle = nullptr;

  int64_t                  m_previousTime{};
  PerformanceClock         m_clock{};
  std::unique_ptr<Device>  m_device = nullptr;

  std::unique_ptr<GraphicalResourceRegistry> m_globalGraphicalResources;
  std::unique_ptr<FrameBuffer>               m_primaryFrameBuffer;
  std::shared_ptr<ScreenResizeEventHandler>  m_windowResizeEventHandler;

  bool m_bShouldExit = false;

  static inline Engine *s_engineSingleton = nullptr;
};

}
