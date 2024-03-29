# LearnVulkan

[![CI Build](https://github.com/lcdavison/LearnVulkan/actions/workflows/CI-Build.yml/badge.svg?branch=main)](https://github.com/lcdavison/LearnVulkan/actions/workflows/CI-Build.yml)
[![Nightly Build](https://github.com/lcdavison/LearnVulkan/actions/workflows/Nightly-Build.yaml/badge.svg?branch=main)](https://github.com/lcdavison/LearnVulkan/actions/workflows/Nightly-Build.yaml)

**WORK IN PROGRESS!**

Learning how to use the Vulkan API by implementing a PBR scene.

## Building

**CMake Version 3.20 or greater is required**

To generate the project run one of the following:
* *GenerateVS2019.bat*
* *GenerateVS2022.bat*

The solution file can then be found in `.\Build`

## Planned Features
- Light Components
- Shadows
- Tiled Forward Lighting
- Scene Graph
- Debug UI

And whatever else I would like to implement in the future...

## Boat Model

Currently the project is hard coded to render the boat model from the following:
https://www.turbosquid.com/3d-models/boat-pbr-model-1522670

For the the model to be loaded correctly, the model data will need to be unzipped and copied to `.\Projects\Vulkan_PBR\Assets\Fishing Boat\`

I will continue to work on this project so this is only temporary, and general model loading will be supported in the future.