Result WaitIdle(const CoreInterface& NRI, Device& device, CommandQueue& commandQueue) {
    Fence* fence = nullptr;
    Result result = NRI.CreateFence(device, 0, fence);
    if (result != Result::SUCCESS)
        return result;

    FenceSubmitDesc fenceSubmitDesc = {};
    fenceSubmitDesc.fence = fence;
    fenceSubmitDesc.value = 1;

    QueueSubmitDesc queueSubmitDesc = {};
    queueSubmitDesc.signalFences = &fenceSubmitDesc;
    queueSubmitDesc.signalFenceNum = 1;

    NRI.QueueSubmit(commandQueue, queueSubmitDesc);
    NRI.Wait(*fence, 1);
    NRI.DestroyFence(*fence);

    return Result::SUCCESS;
}
