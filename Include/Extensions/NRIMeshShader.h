// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(DrawMeshTasksDesc)
{
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

NRI_STRUCT(MeshShaderInterface)
{
    // Draw
    void (NRI_CALL *CmdDrawMeshTasks)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(DrawMeshTasksDesc) drawMeshTasksDesc);
    void (NRI_CALL *CmdDrawMeshTasksIndirect)(NRI_NAME_REF(CommandBuffer) commandBuffer, const NRI_NAME_REF(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride); // buffer contains "DrawMeshTasksDesc" commands
};

NRI_NAMESPACE_END
