RaytracingAccelerationStructure rs : register(t0); //t# - tylko odczty (SHADER RESOURCE VIEW    )
RWTexture2D<float4> image : register(u1); //u# - odczyt i zapis (UNORDERED ACCESS VIEW)

[shader("raygeneration")]
void main()
{

}