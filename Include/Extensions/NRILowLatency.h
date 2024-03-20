// © 2024 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(SwapChain);
NRI_FORWARD_STRUCT(CommandQueue);

// us = microseconds

NRI_ENUM
(
    LatencyMarker, uint8_t,

                             // Should be called:
    SIMULATION_START    = 0, // at the start of the simulation execution each frame, but after the call to "LatencySleep"
    SIMULATION_END      = 1, // at the end of the simulation execution each frame
    RENDER_SUBMIT_START = 2, // at the beginning of the render submission execution each frame (must not span into asynchronous rendering)
    RENDER_SUBMIT_END   = 3, // at the end of the render submission execution each frame
    INPUT_SAMPLE        = 6, // just before the application gathers input data, but between SIMULATION_START and SIMULATION_END (yes, 6!)

    MAX_NUM
);

NRI_STRUCT(LatencySleepMode)
{
    uint32_t minIntervalUs; // minimum allowed frame interval (0 - no frame rate limit)
    bool lowLatencyMode;    // low latency mode enablement
    bool lowLatencyBoost;   // hint to increase performance to provide additional latency savings at a cost of increased power consumption
};

NRI_STRUCT(LatencyReport)
{
                                       // The time stamp written:
    uint64_t inputSampleTimeUs;        // when "SetLatencyMarker(INPUT_SAMPLE)" is called
    uint64_t simulationStartTimeUs;    // when "SetLatencyMarker(SIMULATION_START)" is called
    uint64_t simulationEndTimeUs;      // when "SetLatencyMarker(SIMULATION_END)" is called
    uint64_t renderSubmitStartTimeUs;  // when "SetLatencyMarker(RENDER_SUBMIT_START)" is called
    uint64_t renderSubmitEndTimeUs;    // when "SetLatencyMarker(RENDER_SUBMIT_END)" is called
    uint64_t presentStartTimeUs;       // right before "Present"
    uint64_t presentEndTimeUs;         // right after "Present"
    uint64_t driverStartTimeUs;        // when the first "QueueSubmitTrackable" is called
    uint64_t driverEndTimeUs;          // when the final "QueueSubmitTrackable" hands off from the driver
    uint64_t osRenderQueueStartTimeUs;
    uint64_t osRenderQueueEndTimeUs;
    uint64_t gpuRenderStartTimeUs;     // when the first submission reaches the GPU
    uint64_t gpuRenderEndTimeUs;       // when the final submission finishes on the GPU
};

// Multi-swapchain is supported only by VK
NRI_STRUCT(LowLatencyInterface)
{
    NRI_NAME(Result) (NRI_CALL *SetLatencySleepMode)(NRI_NAME_REF(SwapChain) swapChain, const NRI_NAME_REF(LatencySleepMode) latencySleepMode);
    NRI_NAME(Result) (NRI_CALL *SetLatencyMarker)(NRI_NAME_REF(SwapChain) swapChain, NRI_NAME(LatencyMarker) latencyMarker);
    NRI_NAME(Result) (NRI_CALL *LatencySleep)(NRI_NAME_REF(SwapChain) swapChain); // call once before INPUT_SAMPLE
    NRI_NAME(Result) (NRI_CALL *GetLatencyReport)(const NRI_NAME_REF(SwapChain) swapChain, NRI_NAME_REF(LatencyReport) latencyReport);

    // This function must be used in "low latency" mode instead of "QueueSubmit"
    void (NRI_CALL *QueueSubmitTrackable)(NRI_NAME_REF(CommandQueue) commandQueue, const NRI_NAME_REF(QueueSubmitDesc) queueSubmitDesc, const NRI_NAME_REF(SwapChain) swapChain);
};

NRI_NAMESPACE_END
