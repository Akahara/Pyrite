#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "Directxlib.h"

namespace pyr
{

class FrameBuffer;

using ScreenResizeEventHandler = std::function<void(int, int)>;

enum class DeviceMode {
  WINDOWED,
  FULLSCREEN,
};

class Device {
public:
  Device(DeviceMode mode, HWND hWnd);
  ~Device();

  void resizeWindow(int newWidth, int newHeight);
  void present();

public:
  ID3D11Device &getDevice() const { return *m_device; }
  ID3D11DeviceContext &getImmediateContext() const { return *m_immediateContext; }
  IDXGISwapChain &getSwapChain() const { return *m_swapChain; }
  FrameBuffer &getBackbuffer() const { return *m_backbuffer; }

  static void addWindowResizeEventHandler(std::weak_ptr<ScreenResizeEventHandler> resizeHandler)
    { s_screenResizeEventHandler.push_back(std::move(resizeHandler)); }

  static uint32_t getWinWidth() { return s_winWidth; }
  static uint32_t getWinHeight() { return s_winHeight; }

private:
  ID3D11Device            *m_device = nullptr;
  ID3D11DeviceContext     *m_immediateContext = nullptr;
  IDXGISwapChain          *m_swapChain = nullptr;
  std::unique_ptr<FrameBuffer> m_backbuffer;

  static inline std::vector<std::weak_ptr<ScreenResizeEventHandler>> s_screenResizeEventHandler{};
  static inline uint32_t s_winWidth{};
  static inline uint32_t s_winHeight{};
};

}