![GitHub language count](https://img.shields.io/github/languages/count/KaganBaldiran/FusionFrame-Engine)
![GitHub top language](https://img.shields.io/github/languages/top/KaganBaldiran/FusionFrame-Engine) 
![GitHub last commit](https://img.shields.io/github/last-commit/KaganBaldiran/FusionFrame-Engine)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
# FusionFrame-Engine
Fusion Frame engine is a framework type game engine that focuses on delivering a compact and easy-to-use interface for 3d game development while preserving speed. It also supports different physics algorithms to speed up the game development. It utilizes OpenGL as the graphics API.  

# Rendering Abilities

- Deferred rendering(soon to be clustered deferred as well as forward+ as an option)
- Omnidirectional shadows for point lights
- Linear shadows for directional lights
- PBR-IBL shading
- HDRI and cubemap support
- Deferred and forward mesh instancing 
- Compute shader based particle emitter
- Skeletal animation support
- Deferred ScreenSpace decals

Also planning to implement a voxel based GI soon.

# Physics Abilities and Data Structures

- Oct/Quad tree for general object class
- Half-edge mesh structure
- Easy collision mesh creation from meshes and bounding boxes
- AABB and SAT collisions

Also using half-edge data structure , wrote couple of premature mesh algorithms like subdivision for triangular meshes. 
Mesh utility algorithms like quickhull to create collision boxes are also in the TO-DO list

# Path Tracing

It has a monte-carlo path tracer that's compatible with rest of the engine.
Path tracing is done on a compute shader using a hybrid BVH(top and bottom) for acceleration.
Currently the BVH construction done on the CPU but GPU based construction is in the bucket list for the future.
The estimator can sample emissive lights using NEE(Next Even Estimation) and the analytical lights used in the rastered scenes alike. 

Here are quick demo scenes. Scenes don't belong to me.
![Car](https://github.com/user-attachments/assets/50cce874-e4d4-4e7e-9244-6c9e3ab3dbd2)
![Renderman](https://github.com/user-attachments/assets/c821722c-6099-4c40-98eb-9f566bb88b0b)
![Canons](https://github.com/user-attachments/assets/2704193f-8df0-4c66-b50d-52de6465b900)

## Documentation
The overall API is pretty simple and user friendly.
I'll try to demonstrate some of the functionality to get you started

- Initializing the window and the resources
```cpp
const int width = 1000;
const int height = 1000;

FUSIONCORE::Window ApplicationWindow;
ApplicationWindow.InitializeWindow(width, height, 4, 6, false, "FusionFrame Engine");

FUSIONUTIL::InitializeEngineBuffers();
FUSIONUTIL::DefaultShaders Shaders;
FUSIONUTIL::InitializeDefaultShaders(Shaders);
```

- Importing a HDRI
```cpp
FUSIONCORE::CubeMap cubemap(*Shaders.CubeMapShader);
FUSIONCORE::ImportCubeMap("Resources/rustig_koppie_puresky_2k.hdr", 1024, cubemap, Shaders);
```

- Engine supports both deferred and forward rendering so you can use a gbuffer to use deferred rendering.
```cpp
const FUSIONUTIL::VideoMode mode = FUSIONUTIL::GetVideoMode(FUSIONUTIL::GetPrimaryMonitor());
FUSIONCORE::Gbuffer Gbuffer(mode.width, mode.height);
FUSIONCORE::FrameBuffer ScreenFrameBuffer(mode.width, mode.height);
```

## Current Look

![winter_scene_fusion_frame](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/d25d1d46-5a58-4e8b-a983-e5c705a62c44)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/91eb0c73-84d5-446c-9964-75f523d6073c)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/6c7b0ef3-fdd6-40a3-952d-3c45674a5e9e)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/5cfa5f24-db38-4e7f-a31e-8b170c622d8b)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/863e6313-d569-42a5-ae36-3a443a79d2e8)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/f6ab652a-cad7-4e3b-b869-52614d832b0d)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/b0f429e7-9e8e-4c0e-9b93-6b61ec8a4702)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/8db95ca3-9ef6-4770-b484-c3bf79ca6dc0)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/25126184-6aa0-42a3-ab59-977022807888)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/e4956876-731f-4cfc-94b7-6036e15215b3)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/f33b8b6a-c54d-4bd8-85b2-b7a4e9cc9814)
![image](https://github.com/KaganBaldiran/FusionFrame-Engine/assets/80681941/2a04204b-b5d5-4354-b9a2-fd241b51cb98)


  
