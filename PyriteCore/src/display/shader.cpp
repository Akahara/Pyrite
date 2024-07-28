#include "Shader.h"

#define NOMINMAX
#include <stdexcept>
#include <d3dcompiler.h>
#include <fstream>

#include "engine/Directxlib.h"
#include "GraphicalResource.h"
#include "engine/Engine.h"
#include "InputLayout.h"



namespace pyr
{

struct IncludeManager final : ID3DInclude
{
  STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
  {
    using namespace std::string_literals;
    std::ifstream in{ "res/shaders/"s + pFileName};
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto buf = new char[contents.size()];
    std::ranges::copy(contents, buf);
    *ppData = buf;
    *pBytes = static_cast<UINT>(contents.size());
    return S_OK;
  }
  STDMETHOD(Close)(LPCVOID pData) override
  {
      delete[] static_cast<const char*>(pData);
      return S_OK;
  }
};

struct RawEffect {
  ID3DX11Effect* effect = nullptr;
  ID3DX11EffectTechnique* technique = nullptr;
  ID3DX11EffectPass* pass = nullptr;
  D3DX11_EFFECT_SHADER_DESC effectVSDesc2{};
};

RawEffect makeRawEffect(const std::wstring& path, bool mustSucceed)
{
  auto &device = Engine::d3ddevice();

  ID3DX11Effect *effect = nullptr;
  ID3DBlob *errors = nullptr;
  IncludeManager includes{};

  HRESULT compilationSuccess = D3DX11CompileEffectFromFile(path.c_str(), nullptr, &includes, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 2, &device, &effect, &errors);

  if (compilationSuccess != S_OK) {
    const char* errorMessage = errors
      ? static_cast<const char *>(errors->GetBufferPointer())
      : "Could not compile an effect, no error message";
    if (mustSucceed) {
      PYR_ASSERT(false, errorMessage);
    } else {
      PYR_LOG(LogShader, WARN, "Could not compile an effect: ", errorMessage);
      return {};
    }
  }

  ID3DX11EffectTechnique *technique = effect->GetTechniqueByIndex(0);
  ID3DX11EffectPass *pass = technique->GetPassByIndex(0);

  D3DX11_PASS_SHADER_DESC effectVSDesc;
  pass->GetVertexShaderDesc(&effectVSDesc);
  D3DX11_EFFECT_SHADER_DESC effectVSDesc2;
  effectVSDesc.pShaderVariable->GetShaderDesc(effectVSDesc.ShaderIndex, &effectVSDesc2);

  return { effect, technique, pass, effectVSDesc2 };
}

std::shared_ptr<Effect> ShaderManager::makeEffect(const std::wstring &path, const InputLayout& layout)
{
  auto [effect, technique, pass, effectVSDesc2] = makeRawEffect(path, true);
  ID3D11InputLayout *inputLayout = createVertexLayout(layout, effectVSDesc2.pBytecode, effectVSDesc2.BytecodeLength);

  std::shared_ptr<Effect> e = std::make_shared<Effect>(effect, technique, pass, inputLayout);
  e->m_effectFile = widestring2string(path);
  creationHooks(e);
  return e;
}

void ShaderManager::reloadEffect(Effect &e)
{
  auto [effect, technique, pass, effectVSDesc2] = makeRawEffect(string2widestring(e.getFilePath()), false);
  if (effect == nullptr) return; // Invalid shader code
  DXRelease(e.m_effect);
  e.clearBindingCache();
  e.m_effect = effect;
  e.m_technique = technique;
  e.m_pass = pass;
}

ID3D11InputLayout *ShaderManager::createVertexLayout(const InputLayout& layout, const void *shaderBytecode, size_t bytecodeLength)
{
  if (layout.empty())
        return nullptr;

    ID3D11InputLayout *inputLayout = nullptr;

  DXTry(
      Engine::d3ddevice().CreateInputLayout(
          layout,
          static_cast<UINT>(layout.count()),
          shaderBytecode,
          bytecodeLength,
          &inputLayout),
      "Could not create a vertex input layout");
  return inputLayout;
}

Effect::Effect(Effect &&moved) noexcept
  : m_effect(std::exchange(moved.m_effect, nullptr))
  , m_technique(std::exchange(moved.m_technique, nullptr))
  , m_pass(std::exchange(moved.m_pass, nullptr))
  , m_inputLayout(std::exchange(moved.m_inputLayout, nullptr))
  , m_variableBindingsCache(std::move(moved.m_variableBindingsCache))
  , m_constantBufferBindingsCache(std::move(moved.m_constantBufferBindingsCache))
#ifdef PYR_ISDEBUG
  , m_effectFile(std::exchange(moved.m_effectFile, {}))
#endif
{
}

Effect &Effect::operator=(Effect &&moved) noexcept
{
  m_effect = std::exchange(moved.m_effect, nullptr);
  m_technique = std::exchange(moved.m_technique, nullptr);
  m_pass = std::exchange(moved.m_pass, nullptr);
  m_inputLayout = std::exchange(moved.m_inputLayout, nullptr);
  m_variableBindingsCache = std::move(moved.m_variableBindingsCache);
  m_constantBufferBindingsCache = std::move(moved.m_constantBufferBindingsCache);
#ifdef PYR_ISDEBUG
  m_effectFile = std::exchange(moved.m_effectFile, {});
#endif
  return *this;
}

Effect::~Effect()
{
  DXRelease(m_effect);
  DXRelease(m_inputLayout);
}

void Effect::bind() const
{
  auto &device = Engine::d3dcontext();
  device.IASetInputLayout(m_inputLayout);
  DXTry(m_pass->Apply(0, &device), "Could not bind an effect");
}

void Effect::unbindResources()
{
  auto &context = Engine::d3dcontext();
  constexpr ID3D11ShaderResourceView *emptyResources[10]{};
  context.PSSetShaderResources(0, static_cast<UINT>(std::size(emptyResources)), emptyResources);
}

void Effect::bindTexture(const Texture &texture, const std::string &name) const
{
  DXTry(getVariableBinding(name)->AsShaderResource()->SetResource(texture.getRawTexture()), "Could not bind a texture to an effect");
}

void Effect::bindCubemap(const Cubemap &cubemap, const std::string &name) const
{
  DXTry(getVariableBinding(name)->AsShaderResource()->SetResource(cubemap.getRawCubemap()), "Could not bind a cubemap to an effect");
}

void Effect::bindTextures(const std::vector<ID3D11ShaderResourceView*> &textures, const std::string &name) const
{
  ID3D11ShaderResourceView **rawTextures = const_cast<ID3D11ShaderResourceView **>(textures.data());
  DXTry(getVariableBinding(name)->AsShaderResource()->SetResourceArray(rawTextures, 0, static_cast<uint32_t>(textures.size())), "Could not bind textures to an effect");
}

void Effect::bindSampler(const SamplerState &sampler, const std::string &name) const
{
  DXTry(getVariableBinding(name)->AsSampler()->SetSampler(0, sampler.getRawSampler()), "Could not bind a texture sampler to an effect");
}

ID3DX11EffectVariable *Effect::getVariableBinding(const std::string &name) const
{
  auto el = m_variableBindingsCache.find(name);
  return el != m_variableBindingsCache.end()
      ? el->second 
      : (m_variableBindingsCache[name] = m_effect->GetVariableByName(name.c_str()));
}
ID3DX11EffectConstantBuffer *Effect::getConstantBufferBinding(const std::string &name) const
{
  auto el = m_constantBufferBindingsCache.find(name);
  return el != m_constantBufferBindingsCache.end()
        ? el->second
      : (m_constantBufferBindingsCache[name] = m_effect->GetConstantBufferByName(name.c_str()));
}


}