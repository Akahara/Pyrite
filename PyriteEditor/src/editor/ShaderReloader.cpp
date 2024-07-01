#include "ShaderReloader.h"

#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include <efsw/efsw.hpp>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace pye {

struct ShaderAutoReloaderImpl : efsw::FileWatchListener
{
  using clock = std::chrono::high_resolution_clock;

public:
  ShaderAutoReloaderImpl()
  {
    m_fileWatcher.watch();
  }

  void addEffectToWatch(std::weak_ptr<pyr::Effect> effect)
  {
    std::shared_ptr<pyr::Effect> effectLock = effect.lock();
    if (!effectLock) throw std::runtime_error("Cannot watch a null effect");
    std::string effectDirPath = fs::path(effectLock->getFilePath()).parent_path().string();
    auto it = m_watchedEffects.find(effectDirPath);
    if (it == m_watchedEffects.end()) {
      PYR_LOG(LogShader, INFO, "Watching directory ", effectDirPath, " for shader ", effectLock->getFilePath());
      m_watchedEffects.emplace(effectDirPath, std::vector<std::weak_ptr<pyr::Effect>>{ { effect }});
      
      efsw::WatchID watchId = m_fileWatcher.addWatch(effectDirPath, this);
      m_watchToPath[watchId] = effectDirPath;
    } else {
      PYR_LOG(LogShader, INFO, "Already watching directory ", effectDirPath, " for shader ", effectLock->getFilePath());
      it->second.push_back(effect);
    }
  }

private:
  void handleFileAction(
    efsw::WatchID watchid,
    const std::string &dir,
    const std::string &filename,
    efsw::Action action,
    std::string oldFilename
  ) override
  {
    std::lock_guard _{ m_debouncingMutex };

    bool isDebouncingThreadRunning = !m_pendingChanges.empty();
    bool isShaderFileChange = false;
    for (std::weak_ptr<pyr::Effect> effectPtr : m_watchedEffects.at(m_watchToPath.at(watchid))) {
      if (auto rawPtr = effectPtr.lock()) {
        if (rawPtr->getFilePath().ends_with(filename)) {
          PYR_LOG(LogShader, INFO, "Pickup shader file change: ", rawPtr->getFilePath());
          m_pendingChanges.emplace(std::hash<std::string>{}(rawPtr->getFilePath()), effectPtr);
          isShaderFileChange = true;
        }
      }
    }

    if (!isShaderFileChange)
      return;

    m_changesTimestamp = clock::now();

    if (isDebouncingThreadRunning)
      return;

    if (m_debouncingThread.joinable())
      m_debouncingThread.join();

    m_debouncingThread = std::thread([&] {
      constexpr auto delay = 100ms;
      std::this_thread::sleep_for(delay);

      while (true) {
        std::lock_guard _{ m_debouncingMutex };
        auto now = clock::now();
        if (now > m_changesTimestamp + delay) {
          flushChanges();
          return;
        } else {
          std::this_thread::sleep_for((m_changesTimestamp + delay - now) * 1.1f);
        }
      }
    });
  }

  void flushChanges()
  {
    PYR_LOG(LogShader, INFO, "Flushing changes");

    std::lock_guard _{ pyr::Engine::getFrameMutex() };

    for (auto &[_,effectPtr] : m_pendingChanges) {
      if (auto effectRawPtr = effectPtr.lock()) {
        pyr::Effect &effect = *effectRawPtr;
        PYR_LOG(LogShader, INFO, "Reloading shader ", effect.getFilePath());
        pyr::ShaderManager::reloadEffect(effect);
      }
    }
    m_pendingChanges.clear();
  }

private:
  efsw::FileWatcher m_fileWatcher;
  std::unordered_map<efsw::WatchID, std::string> m_watchToPath;
  std::unordered_map<std::string, std::vector<std::weak_ptr<pyr::Effect>>> m_watchedEffects;

  std::mutex m_debouncingMutex;
  std::unordered_map<size_t, std::weak_ptr<pyr::Effect>> m_pendingChanges; // only a map because unordered_set<weak_ptr> is not a possibility
  std::chrono::time_point<clock> m_changesTimestamp;
  std::thread m_debouncingThread;
};

ShaderAutoReloader::ShaderAutoReloader()
  : m_impl(std::make_unique<ShaderAutoReloaderImpl>())
{
  m_hookHandle = pyr::ShaderManager::creationHooks.add([&](std::shared_ptr<pyr::Effect> &effect) {
    m_impl->addEffectToWatch(effect);
  });
}

ShaderAutoReloader::~ShaderAutoReloader() = default; // necessarily in the .cpp file because of pimpl

}