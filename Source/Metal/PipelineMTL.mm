#include "SharedMTL.h"

#include "PipelineMTL.h" 

using namespace nri;

PipelineMTL::~PipelineMTL() {

}

Result PipelineMTL::Create(const ComputePipelineDesc& computePipelineDesc) {
    return Result::SUCCESS;
}

Result PipelineMTL::Create(const RayTracingPipelineDesc& rayTracingPipelineDesc) {
    return Result::SUCCESS;
}

Result PipelineMTL::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    MTLRenderPipelineDescriptor *renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
   
    m_type = PipelineMTL::Graphics;
    const InputAssemblyDesc& ia = graphicsPipelineDesc.inputAssembly;
    switch (ia.topology) {
    case Topology::POINT_LIST:
        m_graphics.m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypePoint;
        break;
    case Topology::LINE_LIST:
    case Topology::LINE_STRIP:
    case Topology::LINE_LIST_WITH_ADJACENCY:
    case Topology::LINE_STRIP_WITH_ADJACENCY:
        m_graphics.m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypeLine;
        break;
    case Topology::TRIANGLE_LIST:
    case Topology::TRIANGLE_STRIP:
    case Topology::TRIANGLE_LIST_WITH_ADJACENCY:
    case Topology::TRIANGLE_STRIP_WITH_ADJACENCY:
        m_graphics.m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypeTriangle;
        break;
    case Topology::PATCH_LIST:
    default:
        break;
    }

    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
    }
    // Depth-stencil
    const DepthAttachmentDesc& da = graphicsPipelineDesc.outputMerger.depth;
    const StencilAttachmentDesc& sa = graphicsPipelineDesc.outputMerger.stencil;
    
    const PipelineLayout *pl = graphicsPipelineDesc.pipelineLayout;
    const VertexInputDesc *vi = graphicsPipelineDesc.vertexInput;
    if (vi) {
        
      //  VkVertexInputBindingDescription* streams const_cast<VkVertexInputBindingDescription*>(vertexInputState.pVertexBindingDescriptions);
        //for (uint32_t i = 0; i < vi->streamNum; i++) {
        //    const VertexStreamDesc &stream = vi->streams[i];
        //    renderPipelineDesc.vertexDescriptor.layouts[attribute_desc.streamIndex].stride = stream.stride;
        //    renderPipelineDesc.vertexDescriptor.layouts[attribute_desc.streamIndex].stepRate = 1;
       // }
    }
    
    renderPipelineDesc.inputPrimitiveTopology = GetTopologyMTL(ia.topology);
    
    if (graphicsPipelineDesc.multisample) {
        // TODO: multisampling
    }

    // Blending

    // assign render target pixel format for all attachments
    const OutputMergerDesc& om = graphicsPipelineDesc.outputMerger;
    for (uint32_t i = 0; i < om.colorNum; i++) {
        
        const ColorAttachmentDesc& attachmentDesc = om.color[i];
        renderPipelineDesc.colorAttachments[i].pixelFormat = GetFormatMTL(attachmentDesc.format, false);
        
        renderPipelineDesc.colorAttachments[i].blendingEnabled = attachmentDesc.blendEnabled;
        renderPipelineDesc.colorAttachments[i].rgbBlendOperation = GetBlendOp(attachmentDesc.colorBlend.func);
        renderPipelineDesc.colorAttachments[i].alphaBlendOperation = GetBlendOp(attachmentDesc.alphaBlend.func);

        renderPipelineDesc.colorAttachments[i].sourceRGBBlendFactor = GetBlendFactor(attachmentDesc.colorBlend.srcFactor);
        renderPipelineDesc.colorAttachments[i].destinationRGBBlendFactor = GetBlendFactor(attachmentDesc.colorBlend.dstFactor);
        renderPipelineDesc.colorAttachments[i].sourceAlphaBlendFactor = GetBlendFactor(attachmentDesc.alphaBlend.srcFactor);
        renderPipelineDesc.colorAttachments[i].destinationAlphaBlendFactor = GetBlendFactor(attachmentDesc.alphaBlend.dstFactor);

        renderPipelineDesc.colorAttachments[i].writeMask = GetColorComponent(attachmentDesc.colorWriteMask);
        
    }
    return Result::SUCCESS;

}
