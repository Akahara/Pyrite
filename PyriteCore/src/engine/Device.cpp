#include "Device.h"

#include <vector>

#include <dxgitype.h>

#include "Engine.h"
#include "display/FrameBuffer.h"
#include "engine/Directxlib.h"

#include "inputs/UserInputs.h"

#define DO_D3D11_DEBUG

namespace pyr
{

enum class DeviceInfoType
{
  ACTIVE_ADAPTER
};

class DeviceInfo
{
public:
  explicit DeviceInfo(DeviceInfoType adapter);
  explicit DeviceInfo(const DXGI_MODE_DESC& mode);

  bool isValid() const { return m_isValid; }
  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }
  int getMemory() const { return m_memory; }
  const DXGI_MODE_DESC &getMode() { return m_mode; }

private:
  bool m_isValid = false;
  int m_width{};
  int m_height{};
  int m_memory{};
  wchar_t m_gpuName[100]{};
  DXGI_MODE_DESC m_mode{};
};

Device::Device(DeviceMode mode, HWND hWnd)
{
  UINT createDeviceFlags = 0;
#ifdef DO_D3D11_DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  constexpr D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
  constexpr UINT featureLevelCount = ARRAYSIZE(featureLevels);

  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  ZeroMemory(&swapChainDesc, sizeof swapChainDesc);

  switch (mode) {
  case DeviceMode::WINDOWED:
    RECT windowRect;
    if (Engine::getEngineSettings().bHasTitleBar)
      WinTry(GetClientRect(hWnd, &windowRect));
    else
      WinTry(GetWindowRect(hWnd, &windowRect));
    s_winWidth = windowRect.right - windowRect.left;
    s_winHeight = windowRect.bottom - windowRect.top;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = s_winWidth;
    swapChainDesc.BufferDesc.Height = s_winHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    break;
  default:
    throw std::exception("Not implemented: device mode other than WINDOWED");
  }

  swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  RECT rcClient, rcWindow;
  POINT ptDiff;
  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWindow);
  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
  MoveWindow(hWnd, rcWindow.left, rcWindow.top, static_cast<int>(s_winWidth) + ptDiff.x, static_cast<int>(s_winHeight) + ptDiff.y, TRUE);

  DXTry(
    D3D11CreateDeviceAndSwapChain(
      NULL,
      D3D_DRIVER_TYPE_HARDWARE,
      NULL,
      createDeviceFlags,
      featureLevels,
      featureLevelCount,
      D3D11_SDK_VERSION,
      &swapChainDesc,
      &m_swapChain,
      &m_device,
      NULL,
      &m_immediateContext
    ),
    "Could not create a DirectX device");

  D3D11_VIEWPORT viewport;
  viewport.Width = (FLOAT)s_winWidth;
  viewport.Height = (FLOAT)s_winHeight;
  viewport.MinDepth = 0.f;
  viewport.MaxDepth = 1.f;
  viewport.TopLeftX = 0.f;
  viewport.TopLeftY = 0.f;
  m_immediateContext->RSSetViewports(1, &viewport);

  m_backbuffer = std::make_unique<FrameBuffer>(m_swapChain, m_device, s_winWidth, s_winHeight);

  if (!PYR_ENSURE(RenderDoc::Init()))
  {
      PYR_LOG(LogDebug, FATAL, "Could not init renderdoc. Make sure the dll is in /Pyrite/x64/Debug !");
  }
}

Device::~Device()
{
  m_swapChain->SetFullscreenState(FALSE, NULL);
  m_immediateContext->ClearState();
  m_immediateContext->Flush();
  DXRelease(m_swapChain);
  DXRelease(m_immediateContext);
#ifdef DO_D3D11_DEBUG
  ID3D11Debug *debugDev = nullptr;
  DXTry(m_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDev)), "Could not report live objects");
  DXTry(debugDev->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL), "Could not report live objects");
#endif
  DXRelease(m_device);
}

void Device::present()
{
  m_swapChain->Present(0, 0);
}

void Device::resizeWindow(int newWidth, int newHeight)
{
  m_immediateContext->OMSetRenderTargets(0, NULL, NULL);
  m_backbuffer.reset();
  Effect::unbindResources();

  m_immediateContext->ClearState();
  DXTry(m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0), "Could not resize the swap chain buffers");
  m_backbuffer = std::make_unique<FrameBuffer>(m_swapChain, m_device, newWidth, newHeight);

  D3D11_VIEWPORT vp;
  vp.Width  = static_cast<float>(newWidth);
  vp.Height = static_cast<float>(newHeight);
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  m_immediateContext->RSSetViewports(1, &vp);

  s_winWidth = newWidth;
  s_winHeight = newHeight;
  ScreenRegion::SCREEN_WIDTH = ScreenRegion::SCREEN_HEIGHT / newHeight * newWidth;
  for(size_t i = s_screenResizeEventHandler.size(); i > 0; i--) {
    auto handler = s_screenResizeEventHandler[i-1];
    if (auto lock = handler.lock())
      (*lock)(newWidth, newHeight);
    else
      s_screenResizeEventHandler.erase(s_screenResizeEventHandler.begin() + i++ - 1);
  }
}

DeviceInfo::DeviceInfo(DeviceInfoType adapterType)
{
  IDXGIFactory* factory = nullptr;
  IDXGIAdapter* adapter = nullptr;
  IDXGIOutput* output = nullptr;
  m_isValid = false;
  CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
  if (FAILED(factory->EnumAdapters((UINT)adapterType, &adapter))) return;
  if (FAILED(adapter->EnumOutputs(0, &output))) return;
  DXGI_OUTPUT_DESC outDesc;
  output->GetDesc(&outDesc);
  m_width = outDesc.DesktopCoordinates.right - outDesc.DesktopCoordinates.left;
  m_height = outDesc.DesktopCoordinates.bottom - outDesc.DesktopCoordinates.top;
  m_isValid = true;
  DXGI_ADAPTER_DESC Desc;
  adapter->GetDesc(&Desc);
  m_memory = (int)(Desc.DedicatedVideoMemory / 1024 / 1024);
  wcscpy_s(m_gpuName, 100, Desc.Description);
  DXRelease(output);
  DXRelease(adapter);
  DXRelease(factory);
}

DeviceInfo::DeviceInfo(const DXGI_MODE_DESC& modeDesc)
{
  IDXGIFactory *factory = nullptr;
  DXTry(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory), "Could not create a DXGI factory");
  IDXGIAdapter *adapter;
  std::vector<IDXGIAdapter*> adapters;
  for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	adapters.push_back(adapter);
  *this = DeviceInfo(DeviceInfoType::ACTIVE_ADAPTER);
  IDXGIOutput *output = nullptr;
  DXTry(adapters[0]->EnumOutputs(0, &output), "Could not list device adapters");
  DXTry(output->FindClosestMatchingMode(&modeDesc, &m_mode, nullptr), "Could not retrieve a valid device mode");
  DXRelease(output);
  for (size_t i = 0; i < adapters.size(); ++i)
	DXRelease(adapters[i]);
  DXRelease(factory);
}

bool RenderDoc::Init()
{
    // At init, on windows
    if (HMODULE mod = LoadLibraryA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)(&RenderDoc::m_api));
        return ret == 1;
    }
    return false;
}

}