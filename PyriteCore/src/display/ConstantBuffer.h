#pragma once

#define InlineStruct(body) decltype([]() {\
    struct ConstantBufferData_t {\
        body;\
    } _;\
    return _; \
}())\

#include "utils/Math.h"

struct ID3D11Buffer;

namespace pyr

{
    template<typename DataStruct>
    class ConstantBuffer
    {
    private:

        ID3D11Buffer* m_buffer;

    public:

        using data_t = DataStruct;

        ConstantBuffer();
        ConstantBuffer(data_t d) : ConstantBuffer() { setData(d); }

        void setData(const DataStruct& data);
        [[nodiscard]] const ID3D11Buffer* getRawBuffer() const noexcept { return m_buffer; }        
        [[nodiscard]] ID3D11Buffer* getRawBuffer() noexcept { return m_buffer; }        
    };
}

#include "ConstantBuffer.inl"