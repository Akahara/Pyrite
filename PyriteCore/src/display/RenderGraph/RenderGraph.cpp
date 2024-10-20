#include "RenderGraph.h"

#include "engine/Engine.h"
#include "d3d11_1.h"
#include "display/shader.h"

using namespace pyr;

void RenderGraph::execute(const RenderContext& frameRenderContext /* = {}*/) {
    m_renderContext = frameRenderContext;

    ID3DUserDefinedAnnotation* pPerf;
    HRESULT hr = pyr::Engine::d3dcontext().QueryInterface(__uuidof(pPerf), reinterpret_cast<void**>(&pPerf));
    if (FAILED(hr)) return;

    pPerf->BeginEvent(string2widestring(frameRenderContext.debugName).c_str());
    for (RenderPass* p : m_passes)
    {
        if (p->isEnabled())
        {
            pPerf->BeginEvent(string2widestring(p->displayName).c_str());
            p->apply();
            pPerf->EndEvent();
        }
    }
    pPerf->EndEvent();
    DXRelease(pPerf);
}