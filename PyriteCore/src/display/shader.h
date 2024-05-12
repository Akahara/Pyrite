#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "ConstantBuffer.h"
#include "ConstantBufferBinding.h"
#include "Texture.h"
#include "utils/Debug.h"
#include "utils/StringUtils.h"

static inline PYR_DEFINELOG(LogShader, VERBOSE);

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
  void bindTexture(const Texture &texture, const std::string &name) const;
  void bindCubemap(const Cubemap &cubemap, const std::string &name) const;
  void bindTextures(const std::vector<ID3D11ShaderResourceView *> &textures, const std::string &name) const;
  void bindSampler(const SamplerState &sampler, const std::string &name) const;
  
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////:

  void addBinding(const ConstantBufferBinding& binding) { m_bindings.push_back(binding); }

  void uploadAllBindings() const
  {
	  std::ranges::for_each(m_bindings, [this](const ConstantBufferBinding& binding) 
		  { 
			  if (!binding.bufferRef && binding.Flag != ConstantBufferBinding::BindFlag::Ignorable)
			  {
				  PYR_LOGF(LogShader, FATAL, "Constant buffer binding {} has no constant buffer reference.", binding.label);
				  PYR_LOGF(LogShader, WARN, "Constant buffer binding {} has no constant buffer reference.", binding.label);
			  }
			  if (binding.bufferRef) bindConstantBuffer(binding.label, binding.bufferRef);
		  }
	  );
  }

  void bindConstantBuffer(const std::string& constantBufferName, std::shared_ptr<BaseConstantBuffer> data) const
  {
	  ID3DX11EffectConstantBuffer* pCB = m_effect->GetConstantBufferByName(constantBufferName.c_str());
	  pCB->SetConstantBuffer(data->getRawBuffer());
	  DXRelease(pCB);
  }

  // This is the direct way of settings cbuffers value. Consider using a cbufferBinding that basically does this under the hood when calling uploadAllCbuffers
  template<class DataStruct>
  void bindConstantBuffer(const std::string& constantBufferName, const ConstantBuffer<DataStruct>& data) const
  {
	  DXTry(getConstantBufferBinding(constantBufferName)->SetConstantBuffer(const_cast<ID3D11Buffer *>(data.getRawBuffer())), "Could not bind a CBuffer to an effect");
  }

  template<class T> requires std::is_floating_point_v<T>
  void setUniform(const std::string& uniformName, const T& data)
  {
      m_effect->GetVariableByName(uniformName.c_str())->AsScalar()->SetFloat(static_cast<float>(data));
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


  std::vector<ConstantBufferBinding> m_bindings; // todo say bind all cbuffers

};

class ShaderManager
{
public:
  static Effect makeEffect(const std::wstring &path, const InputLayout& layout);

private:
  static ID3D11InputLayout *createVertexLayout(const InputLayout& layout, const void *shaderBytecode, size_t bytecodeLength);
};

}