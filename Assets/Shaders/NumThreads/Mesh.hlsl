cbuffer MatrrixBuffer
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
};

struct Vertex
{
    float4 position;
    float4 color;
};

struct MeshOutput
{
    float4 Pos : SV_POSITION;
};

struct PrimitiveOutput
{
    nointerpolation float4 Color : COLOR0;
};

StructuredBuffer<Vertex> gVertices : register(t0);
StructuredBuffer<uint> gIndices : register(t1);

static const uint kVertexCount = 24;
static const uint kTriangleCount = 12;

float4 ThreadColor(uint id)
{
    switch (id % 16)
    {
        case 0:
            return float4(1.00, 0.00, 0.00, 1.00); // Red
        case 1:
            return float4(0.00, 1.00, 0.00, 1.00); // Green
        case 2:
            return float4(0.00, 0.00, 1.00, 1.00); // Blue
        case 3:
            return float4(1.00, 1.00, 0.00, 1.00); // Yellow
        case 4:
            return float4(1.00, 0.00, 1.00, 1.00); // Magenta
        case 5:
            return float4(0.00, 1.00, 1.00, 1.00); // Cyan

        case 6:
            return float4(1.00, 0.50, 0.00, 1.00); // Orange
        case 7:
            return float4(0.50, 0.00, 1.00, 1.00); // Purple
        case 8:
            return float4(0.00, 0.50, 1.00, 1.00); // Sky Blue
        case 9:
            return float4(0.50, 1.00, 0.00, 1.00); // Lime
        case 10:
            return float4(1.00, 0.25, 0.50, 1.00); // Pink
        case 11:
            return float4(0.25, 1.00, 0.50, 1.00); // Mint

        case 12:
            return float4(0.75, 0.25, 0.00, 1.00); // Brown
        case 13:
            return float4(0.25, 0.75, 1.00, 1.00); // Light Blue
        case 14:
            return float4(0.75, 0.75, 0.75, 1.00); // Light Gray
        default:
            return float4(1.00, 1.00, 1.00, 1.00); // White
    }
}

[outputtopology("triangle")]
[numthreads(12, 1, 1)]
void MS(
    uint threadId : SV_GroupIndex,
    out vertices MeshOutput verts[kVertexCount],
    out indices uint3 tris[kTriangleCount],
    out primitives PrimitiveOutput prims[kTriangleCount]
)
{
    SetMeshOutputCounts(kVertexCount, kTriangleCount);

    uint baseIndex = threadId * 3;

    // Cada hilo procesa los 3 vertices de su triángulo
    for (uint i = 0; i < 3; ++i)
    {
        uint vertexIndex = gIndices[baseIndex + i];

        Vertex v = gVertices[vertexIndex];
        v.position.w = 1.0f;

        MeshOutput outVertex;
        outVertex.Pos = mul(v.position, World);
        outVertex.Pos = mul(outVertex.Pos, View);
        outVertex.Pos = mul(outVertex.Pos, Projection);

        verts[vertexIndex] = outVertex;
    }

    prims[threadId].Color = ThreadColor(threadId);

    if (threadId == 0)
    {
        for (uint i = 0; i < kTriangleCount; ++i)
        {
            uint triBaseIndex = i * 3;

            tris[i] = uint3(
                gIndices[triBaseIndex + 0],
                gIndices[triBaseIndex + 1],
                gIndices[triBaseIndex + 2]
            );
        }
    }
}