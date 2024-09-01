#include "PipelineMTL.h" 
#import <MetalKit/MetalKit.h>

PipelineMTL::~PipelineMTL() {

}

Result PipelineMTL::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    MTLRenderPipelineDescriptor *renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
    }
    
    const VertexInputDesc *vi = graphicsPipelineDesc.vertexInput;
    if (vi) {
        VkVertexInputBindingDescription* streams const_cast<VkVertexInputBindingDescription*>(vertexInputState.pVertexBindingDescriptions);
        for (uint32_t i = 0; i < vi->streamNum; i++) {
            const VertexStreamDesc &stream = vi->streams[i];
            renderPipelineDesc.vertexDescriptor.layouts[attribute_desc.streamIndex].stride = stream.stride;
            renderPipelineDesc.vertexDescriptor.layouts[attribute_desc.streamIndex].stepRate = 1;
        }
    }
    
    const InputAssemblyDesc& ia = graphicsPipelineDesc.inputAssembly;
    renderPipelineDesc.inputPrimitiveTopology = GetTopologyMTL(ia.topology);
    
    if (graphicsPipelineDesc.multisample) {
        // TODO: multisampling
    }


    // Blending
    const OutputMergerDesc& om = graphicsPipelineDesc.outputMerger;

    // assign render target pixel format for all attachments
    for (uint32_t i = 0; i < om.colorNum; i++) {
        
        MTLRenderPipelineColorAttachmentDescriptor& colorAtachment = &renderPipelineDesc.colorAttachments[i]; 
        const ColorAttachmentDesc& attachmentDesc = om.color[i];

        colorAtachment.pixelFormat = GetFormatMTL(attachmentDesc.format, false);
        
        colorAtachment.blendingEnabled = attachmentDesc.blendEnabled;
        colorAtachment.rgbBlendOperation = GetBlendOp(attachmentDesc.colorBlend.func);
        colorAtachment.alphaBlendOperation = GetBlendOp(attachmentDesc.alphaBlend.func);

        colorAtachment.sourceRGBBlendFactor = GetBlendFactor(attachmentDesc.colorBlend.srcFactor);
        colorAtachment.destinationRGBBlendFactor = GetBlendFactor(attachmentDesc.colorBlend.dstFactor);
        colorAtachment.sourceAlphaBlendFactor = GetBlendFactor(attachmentDesc.alphaBlend.srcFactor);
        colorAtachment.destinationAlphaBlendFactor = GetBlendFactor(attachmentDesc.alphaBlend.dstFactor);

        colorAtachment.writeMask = GetColorComponent(pDesc->mColorWriteMasks[blendDescIndex]);
        
    }

}
