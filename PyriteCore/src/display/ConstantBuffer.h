#pragma once

#define InlineStruct(body) decltype([]() {\
    struct ConstantBufferData_t {\
        struct alignas(16) {body;};\
    } _;\
    return _; \
}())\

#include "utils/Math.h"

struct ID3D11Buffer;

namespace pyr

{

    struct BaseConstantBuffer {
    protected:
        
        BaseConstantBuffer() = default;
        ID3D11Buffer* m_buffer = nullptr;

    public:
        [[nodiscard]] const ID3D11Buffer* getRawBuffer() const noexcept { return m_buffer; }
        [[nodiscard]] ID3D11Buffer* getRawBuffer() noexcept { return m_buffer; }
    };


    template<typename DataStruct>
    class ConstantBuffer : public BaseConstantBuffer
    {
    public:

        using data_t = DataStruct;

        ConstantBuffer();
        ~ConstantBuffer() { DXRelease(m_buffer); }
        ConstantBuffer(data_t d) : ConstantBuffer() { setData(d); }

        void setData(const DataStruct& data);
    };

    namespace DefaultBufferCollection {
        using CameraBuffer = pyr::ConstantBuffer <InlineStruct(mat4 mvp; alignas(16) vec3 pos)>;
    }
}

#include "ConstantBuffer.inl"