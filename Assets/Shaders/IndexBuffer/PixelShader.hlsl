struct PixelInputType
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};



float4 PS(PixelInputType input) : SV_TARGET
{
    return input.Color;
}