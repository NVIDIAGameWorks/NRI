# NVIDIA RENDER INTERFACE (NRI)

[![Status](https://github.com/NVIDIAGameWorks/NRI/actions/workflows/build.yml/badge.svg)](https://github.com/NVIDIAGameWorks/NRI/actions/workflows/build.yml)

*NRI* is a low-level abstract render interface which currently supports three backends: D3D11, D3D12 and Vulkan (VK). *NRI* has been designed to support all (at least major) low level features of D3D12 and VK APIs, but at the same time to simplify usage and reduce the amount of code needed (especially compared with VK). *NRI* is written in *C++*, but supports both *C++* and *C* interfaces.

Goals:
- generalization of D3D12 ([spec](https://microsoft.github.io/DirectX-Specs/)) and VK ([spec](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html)) GAPIs
- providing access to low-level features of modern GAPIs
- low overhead
- explicitness
- D3D11 ([spec](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)) support (as much as possible)

Non-goals:
- high level RHI
- exposing entities not existing in GAPIs
- D3D11-like abstraction level
- hidden management of any kind

Key features:
 - *C++* and *C* compatible interfaces
 - generalized common denominator for D3D12, VK and D3D11 GAPIs
 - low overhead
 - ray tracing support
 - mesh shaders support
 - D3D12 Ultimate features support, including enhanced barriers
 - VK [printf](https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/debug_printf.md) support
 - validation layers (GAPI- and NRI- provided)
 - default D3D11 behavior is changed to match D3D12/VK using *NVAPI* or *AMD AGS* libraries, where applicable
 - supporting as much as possible VK-enabled platforms: Windows, Linux, MacOS, Android
 - can be used as a *shared* or *static* library.

 *NRI* is used in:
 - [*NRI samples*](https://github.com/NVIDIAGameWorks/NRISamples)
 - [*NRD Sample*](https://github.com/NVIDIAGameWorks/NRDSample)

## C/C++ INTERFACE DIFFERENCES

| C++                   | C                     |
|-----------------------|-----------------------|
| `nri::Interface`      | `NriInterface`        |
| `nri::Enum::MEMBER`   | `NriEnum_MEMBER`      |
| `nri::CONST`          | `NRI_CONST`           |
| `nri::nriFunction`    | `nriFunction`         |
| `nri::Function`       | `nriFunction`         |
| Reference `&`         | Pointer `*`           |

## ENTITIES

| NRI                     | D3D11                              | D3D12                         | VK                           |
|-------------------------|------------------------------------|-------------------------------|------------------------------|
| `Device`                | `ID3D11Device`                     | `ID3D12Device`                | `VkDevice`                   |
| `CommandBuffer`         | `ID3D11DeviceContext` (deferred)   | `ID3D12CommandList`           | `VkCommandBuffer`            |
| `CommandQueue`          | `ID3D11DeviceContext` (immediate)  | `ID3D12CommandQueue`          | `VkQueue`                    |
| `Fence`                 | `ID3D11Fence`                      | `ID3D12Fence`                 | `VkSemaphore` (timeline)     |
| `CommandAllocator`      | N/A                                | `ID3D12CommandAllocator`      | `VkCommandPool`              |
| `Buffer`                | `ID3D11Buffer`                     | `ID3D12Resource`              | `VkBuffer`                   |
| `Texture`               | `ID3D11Texture`                    | `ID3D12Resource`              | `VkImage`                    |
| `Memory`                | N/A                                | `ID3D12Heap`                  | `VkDeviceMemory`             |
| `Descriptor`            | `ID3D11*View`                      | `D3D12_CPU_DESCRIPTOR_HANDLE` | `Vk*View` or `VkSampler`     |
| `DescriptorSet`         | N/A                                | N/A                           | `VkDescriptorSet`            |
| `DescriptorPool`        | N/A                                | `ID3D12DescriptorHeap`        | `VkDescriptorPool`           |
| `PipelineLayout`        | N/A                                | `ID3D12RootSignature`         | `VkPipelineLayout`           |
| `Pipeline`              | `ID3D11*Shader` and `ID3D11*State` | `ID3D12StateObject`           | `VkPipeline`                 |
| `AccelerationStructure` | N/A                                | `ID3D12Resource`              | `VkAccelerationStructure`    |

## BUILD INSTRUCTIONS

- Install [*Cmake*](https://cmake.org/download/) 3.15+
- Install on
    - Windows: latest *WindowsSDK* and *VulkanSDK*
    - Linux (x86-64): latest *VulkanSDK* and optional *libx11-dev* and *libwayland-dev*
    - Linux (aarch64): optional *libx11-dev* and *libwayland-dev*
- Build (variant 1) - using *Git* and *CMake* explicitly
    - Clone project and init submodules
    - Generate and build the project using *CMake*
- Build (variant 2) - by running scripts:
    - Run `1-Deploy`
    - Run `2-Build`

Notes:
- *Xlib* and *Wayland* can be both enabled
- Minimal supported client is Windows 8.1+. Windows 7 support requires minimal effort and can be added by request

## CMAKE OPTIONS

- `NRI_STATIC_LIBRARY` - build NRI as a static library (`off` by default)
- `NRI_ENABLE_VK_SUPPORT` - enable VULKAN backend (`on` by default)
- `NRI_ENABLE_D3D11_SUPPORT` - enable D3D11 backend (`on` by default on Windows)
- `NRI_ENABLE_D3D12_SUPPORT` - enable D3D12 backend (`on` by default on Windows)

VK only:
- `NRI_ENABLE_XLIB_SUPPORT` - enable *Xlib* support (`on` by default)
- `NRI_ENABLE_WAYLAND_SUPPORT` - enable *Wayland* support (`on` by default)

D3D12 only:
- `NRI_ENABLE_AGILITY_SDK_SUPPORT` - enable Agility SDK (`off` by default)
- `NRI_AGILITY_SDK_PATH` - path to a directory containing Agility SDK: contents of `.nupkg/build/native/` (`C:/AgilitySDK` by default)
- `NRI_AGILITY_SDK_VERSION` - Agility SDK version (`711` or newer)
- `NRI_AGILITY_SDK_DIR` - directory where Agility SDK binaries will be copied to relative to `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (`AgilitySDK` by default)

## AGILITY SDK

*Overview* and *Download* sections can be found [*here*](https://devblogs.microsoft.com/directx/directx12agility/).

D3D12 backend uses Agility SDK to get access to most recent D3D12 features. As soon as these features become available in the OS, installation of Agility SDK will be deprecated.

Installation steps:
- download Agility SDK package
    - `preview` versions require enabling Developer Mode in Windows
- rename `.nupkg` into `.zip`
- extract contents of `.nupkg/build/native` into `NRI_AGILITY_SDK_PATH` folder
    - can be located anywhere
- set `NRI_AGILITY_SDK_DIR` to a path, where `bin` folder needs to be copied
    - most likely closer to the executable
- set `NRI_AGILITY_SDK_VERSION` to the version of the package
- enable `NRI_ENABLE_AGILITY_SDK_SUPPORT`
- re-deploy project
- include auto-generated `NRIAgilitySDK.h` header in the code of your executable, using NRI

## SAMPLES OVERVIEW

NRI samples can be found [*here*](https://github.com/NVIDIAGameWorks/NRISamples).

Samples:
- DeviceInfo - queries and prints out information about device groups in the system
- Clear - minimal example of rendering using framebuffer clears only
- CTest - very simple example of C interface usage
- Triangle - simple textured triangle rendering
- SceneViewer - loading & rendering of meshes with materials
- Readback - getting data from the GPU back to the CPU
- AsyncCompute - demonstrates parallel execution of graphic and compute workloads
- MultiThreading - shows advantages of multi-threaded command buffer recording
- MultiGPU - multi GPU example
- RayTracingTriangle - simple triangle rendeing through ray tracing
- RayTracingBoxes - a more advanced ray tracing example with many BLASes in TLAS
- Wrapper - shows how to wrap native D3D11/D3D12/VK objects into NRI entities
- Resize - demonstrates window resize

## LICENSE

NRI is licensed under the MIT License. This project includes NVAPI software. All uses of NVAPI software are governed by the license terms specified [*here*](https://github.com/NVIDIA/nvapi/blob/main/License.txt).
