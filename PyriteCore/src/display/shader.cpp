#include "shader.h"

#define NOMINMAX
#include <stdexcept>
#include <d3dcompiler.h>
#include <fstream>

#include "engine/directxlib.h"
#include "graphical_resource.h"
#include "engine/windowsengine.h"

namespace pyr
{

template<>
ShaderVertexLayout &ShaderVertexLayout::addField<float>(const char *name, unsigned int count, FieldType fieldType)
{
  DXGI_FORMAT format =
	count == 2 ? DXGI_FORMAT_R32G32_FLOAT :
	count == 3 ? DXGI_FORMAT_R32G32B32_FLOAT :
	count == 4 ? DXGI_FORMAT_R32G32B32A32_FLOAT :
	(DXGI_FORMAT)0;
  return addField(name, format, sizeof(float) * count, fieldType);
}

template<>
ShaderVertexLayout &ShaderVertexLayout::addField<uint32_t>(const char *name, unsigned int count, FieldType fieldType)
{
  DXGI_FORMAT format = count == 1 ? DXGI_FORMAT_R32_UINT : (DXGI_FORMAT)0;
  return addField(name, format, sizeof(uint32_t) * count, fieldType);
}

ShaderVertexLayout::~ShaderVertexLayout() = default;

ShaderVertexLayout &ShaderVertexLayout::addField(const char *name, unsigned int format, unsigned int size, FieldType fieldType)
{
  if (!format) throw std::runtime_error("Tried to add an invalid number of fields in a vertex layout for " + std::string(name));
  D3D11_INPUT_ELEMENT_DESC layoutItem{};
  layoutItem.SemanticName         = name;
  layoutItem.SemanticIndex        = 0;
  layoutItem.Format               = (DXGI_FORMAT)format;
  layoutItem.InputSlot            = fieldType == INSTANCE ? 1 : 0;
  layoutItem.InputSlotClass       = fieldType == INSTANCE ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
  layoutItem.AlignedByteOffset    = fieldType == INSTANCE ? m_instancedStride : m_stride;
  layoutItem.InstanceDataStepRate = fieldType == INSTANCE ? 1 : 0;
  m_layout.push_back(layoutItem);

  if (fieldType == INSTANCE)
		m_instancedStride += size;
  else
		m_stride += size;

  return *this;
}

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

Effect ShaderManager::makeEffect(const std::wstring &path, const ShaderVertexLayout& layout)
{
  auto &device = Engine::d3ddevice();

  ID3DX11Effect *effect = nullptr;
  ID3DBlob *errors = nullptr;
  IncludeManager includes{};

  HRESULT compilationSuccess = D3DX11CompileEffectFromFile(path.c_str(), nullptr, &includes, 0, 0, &device, &effect, &errors);

  if (compilationSuccess != S_OK) {
	  throw std::runtime_error(errors
      ? static_cast<const char *>(errors->GetBufferPointer())
      : "Could not compile an effect, no error message");
  }

  ID3DX11EffectTechnique *technique = effect->GetTechniqueByIndex(0);
  ID3DX11EffectPass *pass = technique->GetPassByIndex(0);

  D3DX11_PASS_SHADER_DESC effectVSDesc;
  pass->GetVertexShaderDesc(&effectVSDesc);
  D3DX11_EFFECT_SHADER_DESC effectVSDesc2;
  effectVSDesc.pShaderVariable->GetShaderDesc(effectVSDesc.ShaderIndex, &effectVSDesc2);
  ID3D11InputLayout *inputLayout = createVertexLayout(layout, effectVSDesc2.pBytecode, effectVSDesc2.BytecodeLength);

  Effect e{ effect, technique, pass, inputLayout };
#ifdef PYR_ISDEBUG
  e.m_effectFile = path;
#endif
  return e;
}

ID3D11InputLayout *ShaderManager::createVertexLayout(const ShaderVertexLayout &layout, const void *shaderBytecode, size_t bytecodeLength)
{
  if (layout.m_layout.empty())
		return nullptr;
	ID3D11InputLayout *inputLayout = nullptr;
  DXTry(
	  Engine::d3ddevice().CreateInputLayout(
		  layout.m_layout.data(),
		  static_cast<UINT>(layout.m_layout.size()),
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

//void Effect::bindBuffer(const GenericBuffer &buffer, const std::string &name) const
//{
//  DXTry(getConstantBufferBinding(name)->SetConstantBuffer(buffer.getRawBuffer()), "Could not bind a constant buffer to an effect");
//}

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