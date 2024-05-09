#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "ConstantBuffer.h"
#include "Texture.h"
#include "utils/Debug.h"
#include "utils/StringUtils.h"

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3DX11Effect;
struct ID3DX11EffectTechnique;
struct ID3DX11EffectPass;
struct ID3DX11EffectVariable;
struct ID3DX11EffectConstantBuffer;
struct D3D11_INPUT_ELEMENT_DESC;

namespace pyr
{

class InputLayout;

class GraphicalResourceRegistry;

class Effect
{
private:
  friend class ShaderManager;
  friend class GraphicalResourceRegistry;

  Effect(ID3DX11Effect *effect, ID3DX11EffectTechnique *technique, ID3DX11EffectPass *pass, ID3D11InputLayout *inputLayout)
	: m_effect(effect), m_technique(technique), m_pass(pass), m_inputLayout(inputLayout) { }
public:

  Effect() = default;
  Effect(Effect &&) noexcept;
  Effect &operator=(Effect &&) noexcept;

  void bind() const;
  static void unbindResources();
  //void bindBuffer(const GenericBuffer &buffer, const std::string &name) const;
  void bindTexture(const Texture &texture, const std::string &name) const;
  void bindCubemap(const Cubemap &cubemap, const std::string &name) const;
  void bindTextures(const std::vector<ID3D11ShaderResourceView *> &textures, const std::string &name) const;
  void bindSampler(const SamplerState &sampler, const std::string &name) const;

  template<class DataStruct>
  void bindConstantBuffer(const std::string& constantBufferName, std::shared_ptr<ConstantBuffer<DataStruct>> data)
  {
	  ID3DX11EffectConstantBuffer* pCB = m_effect->GetConstantBufferByName(constantBufferName.c_str());
	  pCB->SetConstantBuffer(data->getRawBuffer());
      DXRelease(pCB);
  }

  template<class T>
  void setUniform(std::string_view uniformName, T&& data)
       {
  	    //m_effect->GetVariableByName(uniformName.data())->AsScalar()->SetFloat(value);
       }

private:
  ID3DX11EffectVariable *getVariableBinding(const std::string &name) const;
  ID3DX11EffectConstantBuffer *getConstantBufferBinding(const std::string &name) const;

private:
#ifdef PYR_ISDEBUG
  std::wstring m_effectFile;
#endif
  ID3DX11Effect          *m_effect;
  ID3DX11EffectTechnique *m_technique;
  ID3DX11EffectPass      *m_pass;
  ID3D11InputLayout      *m_inputLayout;
  mutable std::unordered_map<std::string, ID3DX11EffectVariable *> m_variableBindingsCache;
  mutable std::unordered_map<std::string, ID3DX11EffectConstantBuffer *> m_constantBufferBindingsCache;
};

class ShaderManager
{
public:
  static Effect makeEffect(const std::wstring &path, const InputLayout& layout);

private:
  static ID3D11InputLayout *createVertexLayout(const InputLayout& layout, const void *shaderBytecode, size_t bytecodeLength);
};

}