#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <span>

#include "ConstantBuffer.h"
#include "ConstantBufferBinding.h"
#include "Texture.h"
#include "utils/Debug.h"
#include "utils/StringUtils.h"
#include "utils/Hooks.h"

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


public:
  struct define_t { std::string name; std::string value; };// Name, value
  Effect(ID3DX11Effect* effect, ID3DX11EffectTechnique* technique, ID3DX11EffectPass* pass, ID3D11InputLayout* inputLayout, const std::vector<Effect::define_t>& defines = {})
	: m_effect(effect), m_technique(technique), m_pass(pass), m_inputLayout(inputLayout), m_defines(defines) { }

  Effect() = default;
  Effect(const Effect &) = delete;
  Effect& operator=(const Effect &) = delete;
  Effect(Effect &&) noexcept;
  Effect &operator=(Effect &&) noexcept;
  ~Effect();

  void bind() const;
  static void unbindResources();
  void bindTexture(const Texture &texture, const std::string &name) const;
  void bindTexture(const TextureArray &texture, const std::string &name) const;
  void bindCubemap(const Cubemap &cubemap, const std::string &name) const;

  void bindCubemaps(const std::vector<pyr::Cubemap>& cubemaps, const std::string &name) const;
  void bindTextures(const std::vector<ID3D11ShaderResourceView *> &textures, const std::string &name) const;
  void bindTextures(const std::vector<pyr::Texture>& textures, const std::string& name) const;

  void bindSampler(const SamplerState &sampler, const std::string &name) const;

  const std::string& getFilePath() const { return m_effectFile; }
  
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
  }

  // This is the direct way of settings cbuffers value. Consider using a cbufferBinding that basically does this under the hood when calling uploadAllCbuffers
  template<class DataStruct>
  void bindConstantBuffer(const std::string& constantBufferName, const ConstantBuffer<DataStruct>& data) const
  {
	  DXTry(getConstantBufferBinding(constantBufferName)->SetConstantBuffer(const_cast<ID3D11Buffer *>(data.getRawBuffer())), "Could not bind a CBuffer to an effect");
  }

  template<class T>
  void setUniform(const std::string& uniformName, const T& data) const
  {
	  setUniformImpl<T>(uniformName, data);
  }

private:
	template<class T>
	void setUniformImpl(const std::string& uniformName, const T& data) const;

	template<>
	void setUniformImpl<bool>(const std::string& uniformName, const bool& data) const
	{
		m_effect->GetVariableByName(uniformName.c_str())->AsScalar()->SetBool(data);
	}

	template<>
	void setUniformImpl<float>(const std::string& uniformName, const float& data) const
	{
		m_effect->GetVariableByName(uniformName.c_str())->AsScalar()->SetFloat(static_cast<float>(data));
	}

	template<>
	void setUniformImpl<vec2>(const std::string& uniformName, const vec2& data) const
	{
		const float vals[2] = { data.x, data.y };
		m_effect->GetVariableByName(uniformName.c_str())->AsVector()->SetFloatVector(vals);
	}

	template<>
	void setUniformImpl<vec3>(const std::string& uniformName, const vec3& data) const
	{
		const float vals[3] = { data.x, data.y, data.z };
		m_effect->GetVariableByName(uniformName.c_str())->AsVector()->SetFloatVector(vals);
	}

	template<>
	void setUniformImpl<vec4>(const std::string& uniformName, const vec4& data) const
	{
		const float vals[4] = { data.x, data.y, data.z, data.w };
		m_effect->GetVariableByName(uniformName.c_str())->AsVector()->SetFloatVector(vals);
	}

	template<>
	void setUniformImpl<mat4>(const std::string& uniformName, const mat4& data) const
	{
		m_effect->GetVariableByName(uniformName.c_str())->AsMatrix()->SetMatrix(data.m[0]);
	}

	template<>
	void setUniformImpl<std::vector<vec4>>(const std::string& uniformName, const std::vector<vec4>& data) const
	{
		m_effect->GetVariableByName(uniformName.c_str())->AsVector()->SetFloatVectorArray(
			reinterpret_cast<const float*>(data.data()),
			0, static_cast<uint32_t>(data.size())
		);
	}

private:
  ID3DX11EffectVariable *getVariableBinding(const std::string &name) const;
  ID3DX11EffectConstantBuffer *getConstantBufferBinding(const std::string &name) const;
  void clearBindingCache() { m_variableBindingsCache.clear(); m_constantBufferBindingsCache.clear(); }

private:
  std::string			 m_effectFile;
  ID3DX11Effect          *m_effect;
  ID3DX11EffectTechnique *m_technique;
  ID3DX11EffectPass      *m_pass;
  ID3D11InputLayout      *m_inputLayout;
  mutable std::unordered_map<std::string, ID3DX11EffectVariable *> m_variableBindingsCache;
  mutable std::unordered_map<std::string, ID3DX11EffectConstantBuffer *> m_constantBufferBindingsCache;

  std::vector<ConstantBufferBinding> m_bindings; // todo say bind all cbuffers
  std::vector<define_t> m_defines;
};

class ShaderManager
{
public:
  using ShaderCreationHook = std::function<void(std::shared_ptr<Effect> &)>;
  using ShaderCreationHookHandle = HookSet<ShaderCreationHook>::HookHandle;

  static std::shared_ptr<Effect> makeEffect(const std::wstring& path, const InputLayout& layout, const std::vector<Effect::define_t>& defines = {});
  static void reloadEffect(Effect& effect);
  static inline HookSet<ShaderCreationHook> creationHooks;

private:
  static ID3D11InputLayout *createVertexLayout(const InputLayout& layout, const void* shaderBytecode, size_t bytecodeLength);
};

}