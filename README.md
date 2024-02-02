# NVIDIA RENDER INTERFACE (NRI)

[![Status](https://github.com/NVIDIAGameWorks/NRI/actions/workflows/build.yml/badge.svg)](https://github.com/NVIDIAGameWorks/NRI/actions/workflows/build.yml)

*NRI* is a low-level abstract render interface which currently supports three backends: D3D11, D3D12 and Vulkan. *NRI* has been designed to support all (at least major) low level features of D3D12 and Vulkan APIs, but at the same time to simplify usage and reduce the amount of code needed (especially compared with Vulkan). *NRI* is written in *C++*, but supports both *C++* and *C* interfaces.

 Key features:
 - *C++* and *C* compatible interfaces
 - common denominator for D3D11, D3D12 and Vulkan APIs
 - low overhead
 - ray tracing support
 - validation layers (GAPI and NRI itself)
 - default D3D11 behavior is changed to match D3D12/VK where applicable using *NVAPI* or *AMD AGS* libraries

 *NRI* is used in:
 - [*NRI samples*](https://github.com/NVIDIAGameWorks/NRISamples)
 - [*NRD Sample*](https://github.com/NVIDIAGameWorks/NRDSample)

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

Vulkan only:
- `NRI_ENABLE_XLIB_SUPPORT` - enable *Xlib* support (`on` by default)
- `NRI_ENABLE_WAYLAND_SUPPORT` - enable *Wayland* support (`on` by default)

D3D12 only:
- `NRI_ENABLE_AGILITY_SDK_SUPPORT` - enable Agility SDK (`off` by default)
- `NRI_AGILITY_SDK_PATH` - path to a directory containing Agility SDK: contents of `.nupkg/build/native/` (`C:/AgilitySDK` by default)
- `NRI_AGILITY_SDK_VERSION` - Agility SDK version (`711` or newer)
- `NRI_AGILITY_SDK_DIR` - directory where Agility SDK binaries will be copied to relative to `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (`AgilitySDK` by default)

## AGILITY SDK

IMPORTANT: NRI features requiring Agility SDK are marked as `// TODO: D3D12 requires Agility SDK` with version specification.

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
