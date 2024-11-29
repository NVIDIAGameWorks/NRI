
#include "SharedMTL.h"

#include "DescriptorSetMTL.h"
#include "DescriptorMTL.h"

using namespace nri;


void DescriptorSetMTL::UpdateDescriptorRanges(uint32_t rangeOffset, uint32_t rangeNum, const DescriptorRangeUpdateDesc* rangeUpdateDescs) {
    
    for(size_t j = 0; j < rangeNum; j++) {
        const DescriptorRangeUpdateDesc& update = rangeUpdateDescs[j];
        
//        uint32_t offset = update.baseDescriptor + descriptorOffset;
        
        for(size_t descIdx = 0; descIdx < update.descriptorNum; descIdx++) {
            
            DescriptorMTL& descriptorImpl = *(DescriptorMTL*)&update.descriptors[descIdx];
            switch(descriptorImpl.GetType()) {
                case DescriptorTypeMTL::IMAGE_VIEW_1D:
                    [m_ArgumentEncoder setTexture: descriptorImpl.GetTextureHandle() atIndex:0];
//                    [m_ArgumentEncoder setTextures:<#(id<MTLTexture>  _Nullable const * _Nonnull)#> withRange:<#(NSRange)#>]
                    break;
                case DescriptorTypeMTL::IMAGE_VIEW_2D:
                    [m_ArgumentEncoder setTexture: descriptorImpl.GetTextureHandle() atIndex:0];
                    break;
                case DescriptorTypeMTL::BUFFER_VIEW:
//                    [m_ArgumentEncoder setBuffer:  offset:<#(NSUInteger)#> atIndex:<#(NSUInteger)#>]
                    break;
            }

//            update.descriptors[descIdx];
        }
    }
}
