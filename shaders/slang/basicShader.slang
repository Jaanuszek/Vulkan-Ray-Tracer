module basicShader;
__include "utils/pc_helpers";
__include "utils/helpers";

struct CameraData
{
    float3 light_pos;
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
    float distance;
    float3 normal;
    float reflector;
    // ========================== SHADOWS
    bool shadowed;
};

public struct HitAttribute
{
    float2 bary;
};

const static uint MAX_RECURSION_DEPTH = 4;
const static half shadowConst = (half)0.3;

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
    // RayPayload payload;
    // ========================== SHADOWS
    // payload.hitValue = float3(1.0, 1.0, 1.0);
    // payload.shadowed = false;
    // // ========================== REFLECTIONS
    float3 color = float3(0, 0, 0);
    float3 throughput = float3(1, 1, 1);
    for (uint i = 0; i < MAX_RECURSION_DEPTH; i++)
    {
        RayPayload payload;
        TraceRay(topLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
        float3 hitColor = payload.hitValue;

        if (payload.distance < 0.0f)
        {
            color += throughput * hitColor;
            break;
        }

        float reflectivity = clamp(payload.reflector, 0.0f, 0.95f);
        color += throughput * hitColor * (1.0f - reflectivity);

        if (reflectivity > 0.001f)
        {
            const float3 hitPos = ray.Origin + ray.Direction * payload.distance;
            ray.Origin = hitPos + payload.normal * 0.01f;
            ray.Direction = reflect(ray.Direction, payload.normal);
            throughput *= reflectivity;
        }
        else
        {
            break;
        }
    }
    outputImage[dispatchIndex.xy] = float4(color, 1.0);
    // outputImage[dispatchIndex.xy] = float4(payload.hitValue.bgr, 1.0);
}

[shader("miss")]
void missShader(inout RayPayload payload)
{
    // ========================== SHADOWS
    // payload.shadowed = false;
    payload.hitValue = float3(0.0, 0.0, 0.0);

    float3 worldRayDir = WorldRayDirection();

    const float3 gradientStart = float3(0.5, 0.6, 1.0);
    const float3 gradientEnd = float3(1.0, 1.0, 1.0);
    float3 unitDir = normalize(worldRayDir);
    float t = 0.5f * (unitDir.y + 1.0f);
    // payload.hitValue = lerp(gradientStart, gradientEnd, t);
    payload.hitValue = (1.0f -t) * gradientStart + t * gradientEnd;
    payload.distance = -1.0f; // oznacza brak trafienia
    payload.normal = float3(0, 0, 0);
    payload.reflector = 0.0f;
}

[shader("closesthit")]
void closestHitShader(inout RayPayload payload, in HitAttribute attribs)
{
    const float3 barycentricCoords = float3(1.0 - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    // payload.hitValue = barycentricCoords; // Color based on barycentric coordinates
    uint instanceIndex = InstanceIndex();
    Triangle tri = unpackTriangle(PrimitiveIndex(), instanceIndex, attribs);
    float4 color = colorSampler.SampleLevel(tri.uv, 0.0);
    // // payload.hitValue = color.bgr; // Color from texture
    // // payload.hitValue = tri.vertices[PrimitiveIndex()].normal; // Color based on normal
    // payload.hitValue = materialBuffer[instanceIndex].albedo.bgr; // Color from material buffer

    if (materialBuffer[instanceIndex].type == MaterialType::METALLIC)
    {
        color = float4(materialBuffer[instanceIndex].albedo.rgb, 1.0);
    }

    // ========================== REFLECTIONS
    float3 normal = tri.normal;
    float3 hitPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 toLight = cameraData.light_pos - hitPosition;
    float lightDistance = length(toLight);
    float3 lightVector = toLight / max(lightDistance, 1e-4);
    float dot_product = max(dot(lightVector, normal), 0.6);
    // payload.hitValue.rgb = materialBuffer[instanceIndex].albedo.rgb * dot_product;
    payload.hitValue.rgb = (color * dot_product).rgb;
    payload.distance = RayTCurrent();
    payload.normal = normal;
    payload.reflector = materialBuffer[instanceIndex].type == MaterialType::METALLIC ? 0.75f : 0.0f;

    // ========================== SHADOWS
    // float3 normal = tri.normal;
    // float3 hitPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // float3 toLight = cameraData.light_pos - hitPosition;
    // float lightDistance = length(toLight);
    // float3 lightVector = toLight / max(lightDistance, 1e-4);
    // float dot_product = max(dot(lightVector, normal), 0.2);

    // float3 baseColor = materialBuffer[instanceIndex].albedo.rgb;
    // float3 shadedColor = baseColor * dot_product;
    // payload.hitValue = shadedColor;

    // RayDesc rayDesc;
    // rayDesc.Origin = hitPosition + normal * 0.01;
    // rayDesc.Direction = lightVector;
    // rayDesc.TMin = 0.001;
    // rayDesc.TMax = max(lightDistance - 0.02, 0.001);

    // payload.shadowed = true;
    // TraceRay(topLevelAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 0, rayDesc, payload);
    // if (payload.shadowed)
    // {
    //     payload.hitValue = shadedColor * shadowConst;
    // }
    // else
    // {
    //     payload.hitValue = shadedColor;
    // }
}