// © 2023 NVIDIA Corporation

#include <memory>

#include <NRIDescs.h>

#ifndef NRI_API_CPP
    #if NRI_STATIC_LIBRARY
        #define NRI_CPP_API
    #else
        #ifdef NRI_GFX_API_WRAPPER_LIBRARY
            #define NRI_API_CPP __declspec( dllexport )
        #else
            #define NRI_API_CPP __declspec( dllimport )
        #endif
    #endif
#endif

struct NRI_API_CPP GfxApiWrapperInterface
{
    using Device = void;
    using Resource = void;
    using CommandList = void;

    virtual Resource* GetNative(nri::Texture const& texture) = 0;
    virtual Resource* GetNative(nri::Buffer const& buffer) = 0;
    virtual CommandList* GetNative(nri::CommandBuffer const& buffer) = 0;

    virtual ~GfxApiWrapperInterface() {}
};

NRI_API_CPP std::shared_ptr<GfxApiWrapperInterface> createGfxApiWrapper(nri::GraphicsAPI graphicsAPI, nri::Device const& device);