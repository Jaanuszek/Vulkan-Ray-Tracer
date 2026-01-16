RaytracingAccelerationStructure rs : register(t0); //t# - tylko odczty (SHADER RESOURCE VIEW    )
[[vk::image_format("rgba8")]]RWTexture2D<float4> image : register(u1); //u# - odczyt i zapis (UNORDERED ACCESS VIEW)

struct Matrices
{
    float4x4 viewInverse; // odwrotna macierz widoku - przejscie z przestrzeni widoku do przestrzeni swiata
    float4x4 projInverse; // odwrotna macierz projekcji - przejscie z przestrzeni NDC do przestrzeni widoku
};

cbuffer camera : register(b2) //b# - stale dane (CONSTANT BUFFER VIEW)
{
    Matrices matrices;
};

// Texture2D<float4> textureSampler : register(t3);
[[vk::combinedImageSampler]]
Texture2D tex : register(t3);
[[vk::combinedImageSampler]][[vk::binding(3)]]
SamplerState viking_sampler : register(s3); //s# - sampler (SAMPLER)

struct Payload
{
    [[vk::location(0)]] float3 hitValue; // do layout 0 zapisujemy kolor
};

[shader("raygeneration")] // to musi byc, explicit mowimy ze to jest rayGen shader
void main()
{
    uint3 launchIndex = DispatchRaysIndex(); // pobieramy indeks pixela w ktorym aktualnie jestesmy
    uint3 launchDim = DispatchRaysDimensions(); // pobieramy rozmiar obrazu (szerokosc, wysokosc) w pikselach
    // indeksowanie zaczyna sie od gornego lewego rogu, x idzie w prawo, y idzie w dol
    // Wiem ze tworzenie zmiennych wewnątrz shaderow nie jest optymalne ze wzgledu na fakt ze te zmienne muszą byc gdzieś przechowywane
    const float2 pixelCenter = launchIndex.xy + float2(0.5, 0.5);
    const float2 UV = pixelCenter / float2(launchDim.xy); // UV w zakresie [0,1]

    float2 NDC = UV * 2.0 - 1.0; // NDC w zakresie [-1,1]
    float4 target = mul(matrices.projInverse, float4(NDC.x, NDC.y, 1.0, 1.0)); // Współrzędne środka piksela w przestrzenii kamery (widoku)
    
    RayDesc ray;
    ray.Origin = mul(matrices.viewInverse, float4(0,0,0,1)).xyz;
    ray.Direction = mul(matrices.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    Payload payload;
    TraceRay(rs, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload); // RAY_FLAG_FORCE_OPAQUE - ignoruje shader any hit

    int width, height;
    tex.GetDimensions(width, height);
    int2 texelCoord = int2(UV.x * width, UV.y * height);
    float4 texColor = tex.Load(int3(texelCoord, 0));

    // image[int2(launchIndex.xy)] = float4(payload.hitValue, 1.0) * texColor;
    image[int2(launchIndex.xy)] = float4(payload.hitValue, 0.0);
}