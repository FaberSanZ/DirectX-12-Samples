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
    
    
    if (id == 0)
    {
        uint i0 = gIndices[0];  // 0
        uint i1 = gIndices[1];  // 1
        uint i2 = gIndices[2];  // 2
        tris[0] = uint3(i0, i1, i2); 
        
        uint i3 = gIndices[3];  // 0
        uint i4 = gIndices[4];  // 2
        uint i5 = gIndices[5];  // 3
        tris[1] = uint3(i3, i4, i5);
    }
}