
# 🌀 DirectX 12 Ultimate® Samples

</br>

📝This repository contains **small, progressive DirectX 12 examples**, designed to teach and demonstrate each part of the graphics pipeline in an intuitive and minimalistic way.  
Each sample is isolated and focused, making it easy to learn DX12 step by step, without unnecessary complexity using Mesh Shaders and Ray Query (DXR Inline).

</br>




## Examples


Example | Details
---------|--------
<img src="Screenshots/ClearScreen.png" width=380> | [Clear Screen](Src/ClearScreen)<br> This sample shows how to initialize DirectX 12, select a GPU (adapter), create a device, a command queue, a swap chain, a render target view (RTV) descriptor heap, and a command list to clear the screen to a solid color. All rendering commands are recorded into the command list, then executed through the command queue. 
<img src="Screenshots/Pipeline.png" width=380> | [Pipeline](Src/Pipeline)<br> We will start drawing geometry onto the screen in this tutorial. We will learn how to use the Mesh Shader Pipeline and DispatchMesh to generate and render geometry directly on the GPU.
<img src="Screenshots/VertexBuffer.png" width=380> | [VertexBuffer](Src/VertexBuffer)<br> Let's get some color in our scene. In this tutorial we will add color to our vertices to color our triangle. This involves updating the mesh shader to pass the color to the pixel shader using vertex pulling, the pixel shader to output the color passed to it, the vertex structure to add a color attribute.
<img src="Screenshots/IndexBuffer.png" width=380> | [IndexBuffer](Src/IndexBuffer)<br> In this tutorial we will learn how to use indices to define our triangles. This is useful because we can remove duplicate vertices, as many times the same vertex is used in multiple triangles using index pulling.
<img src="Screenshots/DepthTests.png" width=380> | [DepthTests](Src/DepthTests)<br> We will create a depth/stencil buffer, then create a depth/stencil view which we bind to the OM stage of the pipeline
<img src="Screenshots/BasicSync.png" width=380> | [Basic Sync](Src/BasicSynchronizing)<br> This example adds basic synchronization using ID3D12Fence to ensure that the GPU has completed its work before reusing resources. It also properly handles window resizing by recreating the Render Target Views (RTVs) and Depth/Stencil Views (DSVs) whenever the swap chain is resized.
<img src="Screenshots/Blending.png" width=380> | [Blending](Src/Blending)<br> Here we will learn about a technique called "blending"! This will give us the ability to render "transparent" primitives.
<img src="Screenshots/ConstantBuffer.png" width=380> | [ConstantBuffer](Src/ConstantBuffer)<br> In this sample, we will learn about matrices, transformations, world/view/projection space matrices, and constant buffers
<img src="Screenshots/ImGui.png" width=380> | [ImGui](Src/ImGui)<br>This sample shows how to use ImGui with DirectX 12 to control two 3D cubes.
<img src="Screenshots/NumThreads.png" width=380> | [NumThreads](Src/NumThreads)<br>This sample introduces `numthreads` in Mesh Shaders. The mesh shader uses multiple threads inside one mesh shader group, with each thread processing one triangle. The first thread writes the output indices, and flat debug colors are used to visualize the work done by each thread.
<img src="Screenshots/RootConstants.png" width=380> | [RootConstants](Src/RootConstants)<br>Uses SetGraphicsRoot32BitConstants, small blocks of uniform data stored within a command list, to pass data to a shader without the need for uniform buffers.




## 📘 Goals

- Help others learn how to use **pure DirectX 12**.
- Serve as a personal reference for building engines or tools.
- Keep things clean, readable, and low-level.

## 🎯 Requirements

- Windows 10/11
- Visual Studio 2019 or newer
- DirectX 12 compatible GPU
- DirectX 12 Ultimate compatible GPU recommended for Mesh Shader and Ray Query samples
