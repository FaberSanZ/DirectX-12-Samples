struct PixelInput
{
    float4 Pos : SV_POSITION;
    nointerpolation float4 Color : COLOR0;
};

float4 PS(PixelInput input) : SV_TARGET
{
    return input.Color;
}