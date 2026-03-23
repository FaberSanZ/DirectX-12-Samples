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
[numthreads(3, 1, 1)]
void MS( uint3 groupThreadID : SV_GroupThreadID, out vertices MeshOutput verts[3], out indices uint3 tris[1])
{
    SetMeshOutputCounts(3, 1);

    uint id = groupThreadID.x;

    uint vertexIndex = gIndices[id];


    // Vertex Pulling
    Vertex v = gVertices[vertexIndex];

    verts[id].Pos = v.position;
    verts[id].Color = v.color;

    // Solo un thread escribe índices
    if (id == 0)
    {
        tris[0] = uint3(0, 1, 2);
    }
}