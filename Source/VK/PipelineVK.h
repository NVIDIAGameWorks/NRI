// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct PipelineLayoutVK;

struct PipelineVK {
    inline PipelineVK(DeviceVK& device) : m_Device(device) {
    }

    inline operator VkPipeline() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkPipelineBindPoint GetBindPoint() const {
        return m_BindPoint;
    }

    ~PipelineVK();

    Result Create(const GraphicsPipelineDesc& graphicsPipelineDesc);
    Result Create(const ComputePipelineDesc& computePipelineDesc);
    Result Create(const RayTracingPipelineDesc& rayTracingPipelineDesc);
    Result CreateGraphics(NRIVkPipeline vkPipeline);
    Result CreateCompute(NRIVkPipeline vkPipeline);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result WriteShaderGroupIdentifiers(uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, void* buffer) const;

  private:
    Result SetupShaderStage(VkPipelineShaderStageCreateInfo& stage, const ShaderDesc& shaderDesc, VkShaderModule*& modules);
    void FillVertexInputState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineVertexInputStateCreateInfo& state) const;
    void FillInputAssemblyState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineInputAssemblyStateCreateInfo& state) const;
    void FillTessellationState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineTessellationStateCreateInfo& state) const;
    void FillViewportState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineViewportStateCreateInfo& state);
    void FillRasterizationState(
        const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineRasterizationStateCreateInfo& state,
        VkPipelineRasterizationConservativeStateCreateInfoEXT& conservativeRasterState
    ) const;
    void FillMultisampleState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineMultisampleStateCreateInfo& state) const;
    void FillDepthStencilState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineDepthStencilStateCreateInfo& state) const;
    void FillColorBlendState(const GraphicsPipelineDesc& graphicsPipelineDesc, VkPipelineColorBlendStateCreateInfo& state) const;
    void FillGroupIndices(const RayTracingPipelineDesc& rayTracingPipelineDesc, uint32_t* groupIndices);
    void FillDynamicState(VkPipelineDynamicStateCreateInfo& state);

  private:
    DeviceVK& m_Device;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    VkPipelineBindPoint m_BindPoint = (VkPipelineBindPoint)0;
    bool m_OwnsNativeObjects = false;
};

} // namespace nri