// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(MeshShaderInterface)
{
    void (NRI_CALL *CmdDispatchMeshTasks)(NRI_NAME_REF(CommandBuffer) commandBuffer, uint32_t x, uint32_t y, uint32_t z);
};

NRI_NAMESPACE_END
