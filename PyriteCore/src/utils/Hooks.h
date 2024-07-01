#pragma once

namespace pyr {

template<class F>
class HookSet {
  using id_t = uint16_t;
  struct Hook {
	id_t id;
	F func;
  };

public:
  class HookHandle {
  private:
	friend class HookSet;
	HookHandle(id_t hookId, std::weak_ptr<std::vector<Hook>> hookSet)
	  : m_hookId(hookId)
	  , m_hookSet(std::move(hookSet))
	{}
  public:
	HookHandle() = default;
	HookHandle(HookHandle &&) = default;
	HookHandle& operator=(HookHandle &&) = default;
	HookHandle(const HookHandle &) = delete;
	HookHandle &operator=(const HookHandle &) = delete;

	~HookHandle()
	{
	  if (auto hooks = m_hookSet.lock()) {
		hooks->erase(std::ranges::find_if(*hooks, [&](Hook &h) { return h.id == m_hookId; }));
	  }
	}

  private:
	id_t m_hookId = -1;
	std::weak_ptr<std::vector<Hook>> m_hookSet;
  };

public:
  [[nodiscard]] HookHandle add(F &&hook)
  {
	id_t hookId = m_nextId++;
	m_hooks->emplace_back(hookId, std::move(hook));
	return HookHandle{ hookId, m_hooks };
  }

  void operator()(auto&&... args)
  {
	std::ranges::for_each(*m_hooks, [&](Hook &h) { h.func(args...); });
  }

private:
  id_t m_nextId = 0;
  std::shared_ptr<std::vector<Hook>> m_hooks = std::make_shared<std::vector<Hook>>();
};

}
