struct Vertex
{
    float4 position;
    float4 color;
};

struct MeshOutput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

// Vertex buffer
StructuredBuffer<Vertex> gVertices : register(t0);
StructuredBuffer<uint> gIndices : register(t1);


[outputtopology("triangle")]
[numthreads(4, 1, 1)]
void MS(uint3 groupThreadID : SV_GroupThreadID, out vertices MeshOutput verts[4], out indices uint3 tris[2])   
{
    SetMeshOutputCounts(4, 2);
    
    uint id = groupThreadID.x;
    

    Vertex v = gVertices[id]; 
    verts[id].Pos = v.position;
    verts[id].Color = v.color;
    
    
    
    for (uint i = 0; i < 2; ++i) // In this example, we are using the first 6 indices to form 2 triangles.
    {
        uint base = i * 3;  // 0, 3

        tris[i] = uint3(gIndices[base + 0], gIndices[base + 1], gIndices[base + 2]);        
    }

    
}