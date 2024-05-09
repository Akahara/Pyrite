#pragma once

#include "ConstantBuffer.h"
#include "engine/Engine.h"

namespace pyr
{
    
template <typename DataStruct>
ConstantBuffer<DataStruct>::ConstantBuffer()
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(data_t);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    DXTry(Engine::d3ddevice().CreateBuffer(&bd, nullptr, &m_buffer), "bruh");
}

template <class DataStruct>
void ConstantBuffer<DataStruct>::setData(const data_t& data)
{
    Engine::d3dcontext().UpdateSubresource(getRawBuffer(), 0, nullptr, &data, 0, 0);
}

}