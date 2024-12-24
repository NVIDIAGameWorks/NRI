#include "SharedMTL.h"

#include "PipelineMTL.h" 
#include "PipelineLayoutMTL.h"

using namespace nri;

PipelineMTL::~PipelineMTL() {
    switch(m_PipelineType) {
        case PipelineType::Graphics:
            m_GraphicsPipeline = nil;
            break;
        case PipelineType::Compute:
            m_ComputePipeline = nil;
            break;
        case PipelineType::Raytracing:
            break;
        default:
            break;
    }

}

Result PipelineMTL::Create(const ComputePipelineDesc& computePipelineDesc) {
    MTLComputePipelineDescriptor* pipelineDesc = [[MTLComputePipelineDescriptor alloc] init];
    NSError* error = nil;
    
    dispatch_data_t byteCode = dispatch_data_create(
                                                    computePipelineDesc.shader.bytecode,
                                                    computePipelineDesc.shader.size, nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    
    id<MTLLibrary> lib = [m_Device newLibraryWithData: byteCode error:&error];
    NSCAssert(lib, @"Failed to load Metal shader library %@", error); // not sure how to correctly report this
    RETURN_ON_FAILURE(&m_Device, lib, Result::FAILURE, "Failed to Load Metal shader library");
    // Create a MTLFunction from the loaded MTLLibrary.
    NSString *entryPointNStr = [lib functionNames][0];
    if (computePipelineDesc.shader.entryPointName) {
        entryPointNStr = [[NSString alloc] initWithUTF8String:computePipelineDesc.shader.entryPointName];
    }
    id <MTLFunction> entryPointFunc = [lib newFunctionWithName:entryPointNStr];
    
    m_PipelineType = PipelineType::Compute;
    pipelineDesc.computeFunction = entryPointFunc;
    NSCAssert(pipelineDesc.computeFunction, @"Failed to create Metal kernel function %@: %@", entryPointNStr, error);
    
    
    m_ComputePipeline = [m_Device newComputePipelineStateWithDescriptor: pipelineDesc
                                                                options: MTLPipelineOptionNone
                                                             reflection: nil
                                                                  error:&error];
    NSCAssert(m_ComputePipeline, @"Failed to create pipeline state: %@", error);
    return Result::SUCCESS;
}

Result PipelineMTL::Create(const RayTracingPipelineDesc& rayTracingPipelineDesc) {
    return Result::SUCCESS;
}

Result PipelineMTL::Create(const GraphicsPipelineDesc& graphicsPipelineDesc) {
    MTLRenderPipelineDescriptor *renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
   
    //m_type = PipelineMTL::Graphics;
    const InputAssemblyDesc& ia = graphicsPipelineDesc.inputAssembly;
    switch (ia.topology) {
        case Topology::POINT_LIST:
            m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypePoint;
            break;
        case Topology::LINE_LIST:
        case Topology::LINE_STRIP:
        case Topology::LINE_LIST_WITH_ADJACENCY:
        case Topology::LINE_STRIP_WITH_ADJACENCY:
            m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypeLine;
            break;
        case Topology::TRIANGLE_LIST:
        case Topology::TRIANGLE_STRIP:
        case Topology::TRIANGLE_LIST_WITH_ADJACENCY:
        case Topology::TRIANGLE_STRIP_WITH_ADJACENCY:
            m_primitiveType = MTLPrimitiveType::MTLPrimitiveTypeTriangle;
            break;
        case Topology::PATCH_LIST:
        default:
            break;
    }

    for (uint32_t i = 0; i < graphicsPipelineDesc.shaderNum; i++) {
        const ShaderDesc& shader = graphicsPipelineDesc.shaders[i];
        m_usedBits |= shader.stage;

        dispatch_data_t byteCode = dispatch_data_create(shader.bytecode, shader.size, nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        NSError *error;
        id<MTLLibrary> lib = [m_Device newLibraryWithData:byteCode error:&error];
        NSCAssert(lib, @"Failed to load Metal shader library %@", error); // not sure how to correctly report this
        RETURN_ON_FAILURE(&m_Device, lib, Result::FAILURE, "Failed to Load Metal shader library");
        
        // Create a MTLFunction from the loaded MTLLibrary.
        NSString *entryPointNStr = [lib functionNames][0];
        if (shader.entryPointName) {
          entryPointNStr = [[NSString alloc] initWithUTF8String:shader.entryPointName];
        }
        id <MTLFunction> entryPointFunc = [lib newFunctionWithName:entryPointNStr];
        if(shader.stage & nri::StageBits::VERTEX_SHADER) {
            renderPipelineDesc.vertexFunction = entryPointFunc;
        } else if(shader.stage & nri::StageBits::FRAGMENT_SHADER) {
            renderPipelineDesc.fragmentFunction = entryPointFunc;
        }
    }
    // Depth-stencil
    const DepthAttachmentDesc& da = graphicsPipelineDesc.outputMerger.depth;
    const StencilAttachmentDesc& sa = graphicsPipelineDesc.outputMerger.stencil;
    
    const PipelineLayout *pl = graphicsPipelineDesc.pipelineLayout;
    const VertexInputDesc *vi = graphicsPipelineDesc.vertexInput;
    if (vi) {
        MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor new];
        for(size_t attIdx = 0; attIdx < vi->attributeNum; attIdx++) {
            const nri::VertexAttributeDesc* attrib = &vi->attributes[attIdx];
            vertexDescriptor.attributes[attrib->mtl.location].offset = attrib->offset;
            vertexDescriptor.attributes[attrib->mtl.location].format = GetVertexFormatMTL(attrib->format);
            vertexDescriptor.attributes[attrib->mtl.location].bufferIndex = attrib->streamIndex;
        }
        
        for(size_t layoutIdx = 0; layoutIdx < vi->streamNum; layoutIdx++) {
            const nri::VertexStreamDesc* stream = &vi->streams[layoutIdx];
            vertexDescriptor.layouts[stream->bindingSlot].stride = stream->stride;
            vertexDescriptor.layouts[stream->bindingSlot].stepRate = static_cast<NSUInteger>(stream->stepRate);
        }
        renderPipelineDesc.vertexDescriptor = vertexDescriptor;
    }
    
    renderPipelineDesc.inputPrimitiveTopology = GetTopologyMTL(ia.topology);
    
    if (graphicsPipelineDesc.multisample) {
        // TODO: multisampling
        //
    }
    
   // renderPipelineDesc.rasterSampleCount = pCreateInfo->pMultisampleState->pSampleMask[0];


    // Blending

    // assign render target pixel format for all attachments
    const OutputMergerDesc& om = graphicsPipelineDesc.outputMerger;
    for (uint32_t i = 0; i < om.colorNum; i++) {
        
        const ColorAttachmentDesc& attachmentDesc = om.colors[i];
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
    NSError* error = nil;
    m_GraphicsPipeline = [m_Device newRenderPipelineStateWithDescriptor:renderPipelineDesc error: &error];
    NSCAssert(m_GraphicsPipeline, @"Failed to create pipeline state: %@", error);

    
    return Result::SUCCESS;

}
