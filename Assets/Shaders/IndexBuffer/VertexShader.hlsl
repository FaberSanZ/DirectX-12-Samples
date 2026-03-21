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
    PixelInputType output;

    output.Pos = input.position;
    output.Color = input.color;


    return output;
}