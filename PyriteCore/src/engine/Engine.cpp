#include "Engine.h"

#include <utility>
#include "Directxlib.h"
#include "display/FrameBuffer.h"
#include "display/RenderProfiles.h"
#include "utils/Clock.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "inputs/UserInputs.h"
#include "scene/SceneManager.h"
#include "utils/StringUtils.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static constexpr const wchar_t *APP_WINDOW_CLASS = L"pyrite_engine_winclass";
static constexpr double FRAME_DELTA = 1./60.;

namespace pyr
{

static LRESULT CALLBACK wndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

Engine::Engine(HINSTANCE hInstance, EngineSettings settings)
  : m_settings(std::move(settings))
  , m_appInstance(hInstance)
{
  s_engineSingleton = this;

  WNDCLASSEX winclass{};
  winclass.cbSize = sizeof(WNDCLASSEX);
  winclass.style = CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc = &wndProcCallback;
  winclass.cbClsExtra = 0;
  winclass.cbWndExtra = 0;
  winclass.hInstance = m_appInstance;
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  winclass.lpszClassName = APP_WINDOW_CLASS;
  RegisterClassEx(&winclass);

  m_mainWindowHandle = CreateWindowW(
    APP_WINDOW_CLASS,
    pyr::string2widestring(m_settings.appTitle).c_str(), 
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    0,
    1600,
    900,
    NULL,
    NULL,
    m_appInstance,
    NULL);

  if (!m_settings.bHasTitleBar)
    SetWindowLong(m_mainWindowHandle, GWL_STYLE, 0);

  if (!m_mainWindowHandle)
    throw std::runtime_error("Could not create a window handle");

  m_accelTable = LoadAccelerators(m_appInstance, APP_WINDOW_CLASS);

  UserInputs::setWindowsHandle(m_appInstance, m_mainWindowHandle);

  m_device = std::make_unique<Device>(DeviceMode::WINDOWED, m_mainWindowHandle);
  m_previousTime = m_clock.getTimeAsCount();
  m_globalGraphicalResources = std::make_unique<GraphicalResourceRegistry>();

  RenderProfiles::initProfiles();
  FrameBufferPipeline::loadGlobalResources(*m_globalGraphicalResources);

  m_primaryFrameBuffer = std::make_unique<FrameBuffer>(Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::Target::COLOR_0 | FrameBuffer::Target::DEPTH_STENCIL);
  m_primaryFrameBuffer->bind();

  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int w, int h) {
    m_primaryFrameBuffer->unbind();
    m_primaryFrameBuffer = std::make_unique<FrameBuffer>(w, h, FrameBuffer::Target::COLOR_0 | FrameBuffer::Target::DEPTH_STENCIL);
    m_primaryFrameBuffer->bind();
  }));

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui_ImplWin32_Init(m_mainWindowHandle);
  ImGui_ImplDX11_Init(&d3ddevice(), &d3dcontext());

  showWindow();
  UserInputs::loadGlobalResources();
}

Engine::~Engine()
{
  m_primaryFrameBuffer->unbind();

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  s_engineSingleton = nullptr;
}

void Engine::run()
{
  if (!SceneManager::getInstance().getActiveScene())
    SceneManager::getInstance().setInitialScene(std::make_unique<EmptyScene>());

  bool running = true;
  while (running && !m_bShouldExit) {
    running &= pollEvents();
    if (!running) return;

    int64_t currentTime = m_clock.getTimeAsCount();
    double delta = m_clock.getDeltaSeconds(m_previousTime, currentTime);
    if (delta <= FRAME_DELTA) continue;

    postFrame();
    runFrame(static_cast<float>(delta));
    
    // if scenes need to be created/deleted drop the frames instead of trying to catch back
    if (SceneManager::getInstance().doSceneTransition())
      m_previousTime = m_clock.getTimeAsCount();
    else
      m_previousTime = currentTime;
  }
}

void Engine::runFrame(float deltaTime)
{
  // update
  UserInputs::pollEvents();
  SceneManager::getInstance().update(deltaTime);

  // render
  FrameBuffer::getActiveFrameBuffer().clearTargets();
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
  SceneManager::getInstance().render();

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Engine::postFrame()
{
  m_device->getBackbuffer().bind();
  FrameBufferPipeline::doSimpleBlit(m_primaryFrameBuffer->getTargetAsTexture(FrameBuffer::COLOR_0));
  Effect::unbindResources();
  m_device->getBackbuffer().unbind();
  m_device->present();
}

bool Engine::pollEvents()
{
  MSG msg;
  bool didUserQuit = false;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY) {
      didUserQuit = true;
    } else if (!TranslateAccelerator(msg.hwnd, m_accelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return !didUserQuit;
}

static LRESULT CALLBACK wndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    return true;

  switch (message) {
  case WM_DESTROY:
    PostQuitMessage(0);
  break;
  case WM_SETFOCUS:
    UserInputs::setNextPollRewireInputs();
    break;
  case WM_KILLFOCUS:
    UserInputs::setInputsNotAquired();
    break;
  case WM_SIZE: {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    if (Engine::hasDevice())
      Engine::device().resizeWindow(width, height);
    break;
  }
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

void Engine::showWindow()
{
  ShowWindow(m_mainWindowHandle, SW_SHOWNORMAL);
  UpdateWindow(m_mainWindowHandle);
}

}