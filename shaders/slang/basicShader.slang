
struct CameraData
{
    column_major float4x4 viewInverse;
    column_major float4x4 projInverse;
};

struct Vertex
{
    float3 pos;
    float2 uv;
};

struct Triangle
{
    Vertex[3] vertices;
    float2 uv;
}

struct pc_blas
{
    uint64_t verticesAddress;
    uint64_t indicesAddress;
};


[[vk::binding(0, 0)]]
uniform RaytracingAccelerationStructure topLevelAS;

[[vk::binding(1, 0)]]
[vk::image_format("rgba8")] RWTexture2D<float4> outputImage;

[[vk::binding(2, 0)]]
ConstantBuffer<CameraData> cameraData;

[[vk::binding(3, 0)]]
uniform Sampler2D colorSampler;

[[vk::push_constant]] pc_blas blasAddr;

struct RayPayload
{
    float3 hitValue;
};

struct HitAttribute
{
    float2 bary;
};

func unpackTriangle(uint idx, HitAttribute attribs) -> Triangle
{
    Triangle tri = {};

    // kazdy trojkat ma 3 wierzcholki, wiec kazdy trojkat to wieloktrontosc 3 wierzcholkow
    const uint triIdx = idx * 3;
    const uint vertexSize = sizeof(Vertex);

    // iteracja po 3 kolejnych wierzchołkach
    for (uint i = 0; i < 3; i++)
    {
        // lokalny indeks wierzchołka (indeks trójkąta plus indeks wierzchołka)
        const uint localVertexIdx = triIdx + i;
        
        // Rzutowanie raw adresu na wskaźnik
        // uint64_t -> uint*
        // rzutujemy na uint, bo indeksy są uint na CPU
        // blasAddr.indicesAddress to adres w pamięci GPU (uint64_t)
        const uint64_t indexAddress = blasAddr.indicesAddress + localVertexIdx * sizeof(uint);
        // Rzutowanie na konkretny indeks Address + offset = adres wskazujący na indeks wierzchołka
        const uint* indexPtr = (const uint*)indexAddress; 
        // dereferencja wskaźnika, aby uzyskać wartość indeksu wierzchołka
        const uint vertexIdx = *indexPtr; 
        
        // Offset wierzchołka
        // blasAddr.verticesAddress to adres w pamięci GPU (uint64_t)
        // dodajemy do tego indeks wierzchołka razy rozmiar struktury Vertex, aby uzyskać adres konkretnego wierzchołka
        // vertexIdx*vertexSize to jest offset wierzchołka(!) w adresie GPU
        const uint64_t vertexOffset = blasAddr.verticesAddress + vertexIdx * vertexSize;

        // Rzutowanie uint64_t na float*, aby uzyskać wskaźnik do danych wierzchołka
        float3* posPtr = (float3*)vertexOffset;
        tri.vertices[i].pos = *posPtr;
        
        float2* uvPtr = (float2*)(vertexOffset + sizeof(float3));
        tri.vertices[i].uv = *uvPtr;
    }
    
    float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    return tri;
}


[shader("raygeneration")]
void rayGenShader()
{
    const uint3 dispatchIndex = DispatchRaysIndex();
    const uint3 dispatchDim = DispatchRaysDimensions();

    const float2 pixelCenter = float2(dispatchIndex.xy) + float2(0.5);
    const float2 UV = pixelCenter / dispatchDim.xy; // [0,1] range

    const float4 camPosView = float4(0, 0, 0, 1); // pozycja kamery w przestrzeni kamery (zawsze 0,0,0)

    float2 ndc = UV * 2.0 - 1.0; // [-1,1] range
    float4 target = mul(cameraData.projInverse, float4(ndc.x, ndc.y, 0, 1)); // przestrzen kamery

    const float3 camDirView = float3(target.xyz - camPosView.xyz); // kierunek z kamery do celu w przestrzeni kamery

    float3 origin = mul(cameraData.viewInverse, camPosView).xyz; // przestrzen swiata
    float3 direction = mul(cameraData.viewInverse, float4(normalize(camDirView), 0)).xyz; // przestrzen swiata

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload;
    TraceRay(topLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);

    outputImage[dispatchIndex.xy] = float4(payload.hitValue.bgr, 0.0);
}

[shader("miss")]
void missShader(inout RayPayload payload)
{
    payload.hitValue = float3(1.0, 1.0, 1.0);   
}

[shader("closesthit")]
void closestHitShader(inout RayPayload payload, in HitAttribute attribs)
{
    // const float3 barycentricCoords = float3(1.0 - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    // payload.hitValue = barycentricCoords; // Color based on barycentric coordinates
    Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    float4 color = colorSampler.SampleLevel(tri.uv, 0.0);
    payload.hitValue = color.bgr;
}