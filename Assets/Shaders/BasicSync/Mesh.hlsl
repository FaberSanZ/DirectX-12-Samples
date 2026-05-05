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

StructuredBuffer<Vertex> gVertices : register(t0);
StructuredBuffer<uint> gIndices : register(t1);

static const uint kVertexCount = 9;
static const uint kTriangleCount = 3;

[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void MS(uint tid : SV_GroupThreadID, out vertices MeshOutput verts[kVertexCount], out indices uint3 tris[kTriangleCount])
{
    SetMeshOutputCounts(kVertexCount, kTriangleCount);

    for (uint i = 0; i < kVertexCount; ++i)
    {
        Vertex v = gVertices[i];
        verts[i].Pos = v.position;
        verts[i].Color = v.color;
    }

    for (uint i = 0; i < kTriangleCount; ++i)
    {
        uint baseIndex = i * 3;
        tris[i] = uint3(gIndices[baseIndex + 0], gIndices[baseIndex + 1], gIndices[baseIndex + 2]);
    }
}
