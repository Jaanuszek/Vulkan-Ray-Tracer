module basicShader;
__include "utils/pc_helpers";

struct CameraData
{
    column_major float4x4 viewInverse;
    column_major float4x4 projInverse;
};

[[vk::binding(0, 0)]]
uniform RaytracingAccelerationStructure topLevelAS;

[[vk::binding(1, 0)]]
[vk::image_format("rgba8")] RWTexture2D<float4> outputImage;

[[vk::binding(2, 0)]]
ConstantBuffer<CameraData> cameraData;

[[vk::binding(3, 0)]]
uniform Sampler2D colorSampler;

[[vk::binding(4, 0)]]
StructuredBuffer<GeometryInfo, Std140DataLayout> geometryInfoBuffer;

[[vk::binding(5, 0)]]
StructuredBuffer<Material, Std140DataLayout> materialBuffer;

[[vk::push_constant]] internal pc_blas blasAddr;

public struct RayPayload
{
    float3 hitValue;
};

public struct HitAttribute
{
    float2 bary;
};

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
    const float3 barycentricCoords = float3(1.0 - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    // payload.hitValue = barycentricCoords; // Color based on barycentric coordinates
    uint instanceIndex = InstanceIndex();
    Triangle tri = unpackTriangle(PrimitiveIndex(), instanceIndex, attribs);
    float4 color = colorSampler.SampleLevel(tri.uv, 0.0);
    // payload.hitValue = color.bgr; // Color from texture
    payload.hitValue = materialBuffer[instanceIndex].albedo.bgr; // Color from material buffer

    // payload.hitValue = barycentricCoords; 
    // uint* vboAddr = (uint*)geometryInfoBuffer[instanceIndex].vertexBufferAddr;

    // if (instanceIndex == 0) {
    //     if (vboAddr != nullptr) // if not null
    //     {
    //         float temp = vboAddr[0];
    //         payload.hitValue = float3(temp, 0.0, 0.0);
    //     }
    //     else
    //     {
    //         // Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    //         // float4 color = colorSampler.SampleLevel(tri.uv, 0.0);
    //         payload.hitValue = barycentricCoords; // Color from texture
    //     }
    // }
    // else if (instanceIndex == 1) {
    //     if (vboAddr != nullptr) // if not null
    //     {
    //         payload.hitValue = float3(0.0, 0.0, 1.0);
    //     }
    //     else
    //     {
    //         // Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    //         // float4 color = colorSampler.SampleLevel(tri.uv, 0.0);
    //         payload.hitValue = barycentricCoords; // Color from texture
    //     }
    // }
    // else {
    //     payload.hitValue = float3(0.0, 1.0, 1.0); // Blue for other instances
    // }
}