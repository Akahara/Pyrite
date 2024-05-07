#pragma once

#include <unordered_map>
#include <vector>

#include "texture.h"
#include "utils/debug.h"

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

class GraphicalResourceRegistry;

struct ShaderVertexLayout
{
  enum FieldType { VERTEX, INSTANCE };

  ~ShaderVertexLayout();

  template<class T>
  ShaderVertexLayout &addField(const char *name, unsigned int count, FieldType fieldType=VERTEX);

private:
  ShaderVertexLayout &addField(const char *name, unsigned int format, unsigned int size, FieldType fieldType);

  friend class ShaderManager;

  std::vector<D3D11_INPUT_ELEMENT_DESC> m_layout;
  unsigned int                          m_stride = 0;
  unsigned int                          m_instancedStride = 0;
};

class Effect
{
private:
  friend class ShaderManager;
  friend class GraphicalResourceRegistry;
  Effect(ID3DX11Effect *effect, ID3DX11EffectTechnique *technique, ID3DX11EffectPass *pass, ID3D11InputLayout *inputLayout)
	: m_effect(effect), m_technique(technique), m_pass(pass), m_inputLayout(inputLayout) { }
public:
  Effect() = default;

  Effect(const Effect &) = delete;
  Effect &operator=(const Effect &) = delete;
  Effect(Effect &&) noexcept;
  Effect &operator=(Effect &&) noexcept;

  void bind() const;
  static void unbindResources();
  //void bindBuffer(const GenericBuffer &buffer, const std::string &name) const;
  void bindTexture(const Texture &texture, const std::string &name) const;
  void bindCubemap(const Cubemap &cubemap, const std::string &name) const;
  void bindTextures(const std::vector<ID3D11ShaderResourceView *> &textures, const std::string &name) const;
  void bindSampler(const SamplerState &sampler, const std::string &name) const;

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
  static Effect makeEffect(const std::wstring &path, const ShaderVertexLayout& layout);

private:
  static ID3D11InputLayout *createVertexLayout(const ShaderVertexLayout &layout, const void *shaderBytecode, size_t bytecodeLength);
};

}