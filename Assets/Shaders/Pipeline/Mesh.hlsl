
[outputtopology("triangle")]
[numthreads(1, 1, 1)]
void MS(uint tid : SV_GroupThreadID, out vertices float4 verts[3] : SV_Position, out indices uint3 tris[1])
{
    SetMeshOutputCounts(3, 1);

    verts[0] = float4( 0.0f,  0.5f, 0.0f, 1.0f);
    verts[1] = float4( 0.5f, -0.5f, 0.0f, 1.0f);
    verts[2] = float4(-0.5f, -0.5f, 0.0f, 1.0f);

    tris[0] = uint3(0, 1, 2);
}