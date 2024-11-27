#include "SharedMTL.h"

#include "SwapChainMTL.h"

using namespace nri;


SwapChainMTL::~SwapChainMTL() {
    
}

Result SwapChainMTL::Create(const SwapChainDesc& swapChainDesc) {
    m_MetalLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;
    
}
