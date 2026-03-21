cbuffer MatrrixBuffer
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
};

struct VertexInputType
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};



PixelInputType VS(VertexInputType input)
{
    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f; // Ensure w is set to 1 for proper transformation ?
    
    PixelInputType output;

    output.Pos = mul(input.position, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    
    output.Color = input.color;


    return output;
}