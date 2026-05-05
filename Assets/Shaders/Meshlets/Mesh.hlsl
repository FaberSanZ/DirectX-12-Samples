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

struct Meshlet
{
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
};

struct MeshOutput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

StructuredBuffer<Meshlet> gMeshlets : register(t0);
StructuredBuffer<uint> gMeshletVertices : register(t1);
StructuredBuffer<uint> gMeshletTriangles : register(t2);
StructuredBuffer<Vertex> gVertices : register(t3);

float4 MeshletColor(uint meshletId)
{
    switch (meshletId % 16)
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
            return float4(0.80, 0.80, 0.80, 1.00); // Light Gray
        default:
            return float4(1.00, 1.00, 1.00, 1.00); // White
    }
}

[outputtopology("triangle")]
[numthreads(12, 1, 1)]
void MS(uint threadId : SV_GroupIndex, uint3 groupId : SV_GroupID, out vertices MeshOutput verts[24], out indices uint3 tris[12])
{
    Meshlet meshlet = gMeshlets[groupId.x];

    SetMeshOutputCounts(meshlet.vertex_count, meshlet.triangle_count);

    float4 meshletColor = MeshletColor(groupId.x);

    for (uint i = threadId; i < meshlet.vertex_count; i += 12)
    {
        uint vertexIndex = gMeshletVertices[meshlet.vertex_offset + i];
        Vertex v = gVertices[vertexIndex];
        v.position.w = 1.0f;

        MeshOutput outVertex;
        outVertex.Pos = mul(v.position, World);
        outVertex.Pos = mul(outVertex.Pos, View);
        outVertex.Pos = mul(outVertex.Pos, Projection);

        // IMPORTANTE:
        // todos los vertices del meshlet reciben el mismo color
        outVertex.Color = meshletColor;

        verts[i] = outVertex;
    }

    if (threadId < meshlet.triangle_count)
    {
        uint packedTriangle = gMeshletTriangles[meshlet.triangle_offset + threadId];

        tris[threadId] = uint3(
            packedTriangle & 0xff,
            (packedTriangle >> 8) & 0xff,
            (packedTriangle >> 16) & 0xff
        );
    }
}