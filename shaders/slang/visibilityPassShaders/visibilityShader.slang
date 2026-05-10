module visibilityShader;

#include "../utils/helpers.slang"
#include "../utils/constants.slang"
#include "../utils/common_structs.slang"

[[vk::binding(0, 0)]]
uniform RaytracingAccelerationStructure topLevelAS;

[[vk::binding(2, 0)]]
ConstantBuffer<CameraData> cameraData;

[[vk::binding(4, 0)]]
StructuredBuffer<GeometryInfo, Std140DataLayout> geometryInfoBuffer;

[[vk::binding(5, 0)]]
StructuredBuffer<Material, Std140DataLayout> materialBuffer;

[[vk::binding(6, 0)]]
StructuredBuffer<float4> cudaColorBuffer;

[[vk::binding(7, 0)]]
StructuredBuffer<uint32_t> triToPatchBuffer;

[[vk::binding(8, 0)]]
RWStructuredBuffer<Patch, Std140DataLayout> patchBuffer;

[[vk::binding(9, 0)]]
RWStructuredBuffer<SelectedPatch, Std140DataLayout> selectedPatchesBuffer;

[[vk::binding(10, 0)]]
RWStructuredBuffer<PatchVisibility, Std140DataLayout> patchVisibilityBuffer;

static const float PI = float.getPi();

float3 safeNormalize(float3 v, float3 fallback)
{
    float len2 = dot(v, v);
    if (len2 < 1e-10f)
    {
        return fallback;
    }
    return v * rsqrt(len2);
}

float3 sampleHemisphere(float2 seed, float3 normal)
{
    float3 d = rand_unit_vec(seed);
    if (dot(d, normal) < 0.0f)
    {
        d = -d;
    }
    return safeNormalize(d, normal);
}

float3 sampleCosineHemisphere(float2 u)
{
    float r = sqrt(u.x);
    float theta = 2.0f * PI * u.y;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0f - u.x);
    return float3(x, y, z);
}

void buildOrthonormalBasis(float3 n, out float3 t, out float3 b)
{
    if (abs(n.z) < 0.999f)
    {
        t = normalize(cross(n, float3(0, 0, 1)));
    }
    else
    {
        t = normalize(cross(n, float3(0, 1, 0)));
    }

    b = cross(n, t);
}

float3 sampleCosineHemisphereWorld(float2 u, float3 normal)
{
    float3 localDir = sampleCosineHemisphere(u);

    float3 t, b;
    buildOrthonormalBasis(normal, t, b);

    return normalize(
        localDir.x * t +
        localDir.y * b +
        localDir.z * normal
    );
}

float radicalInverseVdC(uint bits)
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
    bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
    bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 hammersley2D(uint i, uint n)
{
    float invN = 1.0f / max(float(n), 1.0f);
    return float2((float(i) + 0.5f) * invN, radicalInverseVdC(i));
}

float hash11(uint n)
{
    n ^= 2747636419u;
    n *= 2654435769u;
    n ^= n >> 16;
    n *= 2654435769u;
    n ^= n >> 16;
    n *= 2654435769u;
    return float(n) * 2.3283064365386963e-10f;
}

float2 hash22(uint n)
{
    return float2(hash11(n * 0x9E3779B9u + 0x7F4A7C15u), hash11(n * 0x85EBCA6Bu + 0xC2B2AE35u));
}

[shader("raygeneration")]
void rayGenShader()
{
    const uint3 dispatchIndex = DispatchRaysIndex();
    const uint3 dispatchDim = DispatchRaysDimensions();
    const uint32_t visibilityBufferIndex = dispatchIndex.y * dispatchDim.x + dispatchIndex.x;

    uint32_t selectedPatchID = selectedPatchesBuffer[dispatchIndex.y].patchId;
    if (selectedPatchID == 0xFFFFFFFF)
    {
        patchVisibilityBuffer[visibilityBufferIndex].srcPatchId = 0xFFFFFFFF;
        patchVisibilityBuffer[visibilityBufferIndex].dstPatchId = 0xFFFFFFFF;
        patchVisibilityBuffer[visibilityBufferIndex].visibility = 0.0f;
        return;
    }

    Patch patch = patchBuffer[selectedPatchID];

    float3 normal = patch.normal;

    float2 uv = hash22(dispatchIndex.x + selectedPatchID);
    float offset = sqrt(patch.area) * 0.5f;
    // float3 origin = patch.center + normal * patch.area * 0.003f;
    float3 origin = patch.center + normal * offset;


    RayDesc ray;
    ray.Origin = origin;
    ray.TMin = 0.0005f;
    ray.TMax = 1000.0f;

    visibilityRayPayload payload;
    payload.distance = -1.0f;
    payload.dstPatchId = 0xFFFFFFFF;
    payload.srcPatchId = selectedPatchID;

    uint32_t rayCount = max(dispatchDim.x, 1u);
    float2 seed = hammersley2D(dispatchIndex.x, rayCount);
    seed = frac(seed + hash22(selectedPatchID));
    ray.Direction = sampleCosineHemisphereWorld(seed, normal);

    TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    patchVisibilityBuffer[visibilityBufferIndex].srcPatchId = selectedPatchID;

    if (payload.distance > 0.0f)
    {
        patchVisibilityBuffer[visibilityBufferIndex].dstPatchId = payload.dstPatchId;
        patchVisibilityBuffer[visibilityBufferIndex].visibility = 1.0f;
    }
    else
    {
        patchVisibilityBuffer[visibilityBufferIndex].dstPatchId = 0xFFFFFFFF;
        patchVisibilityBuffer[visibilityBufferIndex].visibility = 0.0f;
    }
}

[shader("closesthit")]
void closestHitShader(inout visibilityRayPayload payload, in HitAttribute attribs)
{
    uint instanceIndex = InstanceIndex();
    uint triangleIndex = PrimitiveIndex();
    uint triGlobalIndex = geometryInfoBuffer[instanceIndex].triToPatchOffset + triangleIndex;
    uint patchId = triToPatchBuffer[triGlobalIndex];

    payload.distance = RayTCurrent();
    payload.dstPatchId = patchId;
}

[shader("anyhit")]
void anyHitShader(inout visibilityRayPayload payload, in HitAttribute attribs)
{
    uint instanceIndex = InstanceIndex();
    uint triangleIndex = PrimitiveIndex();
    uint triGlobalIndex = geometryInfoBuffer[instanceIndex].triToPatchOffset + triangleIndex;
    uint patchId = triToPatchBuffer[triGlobalIndex];

    if (patchId == payload.srcPatchId)
    {
        IgnoreHit();
    }
}

[shader("miss")]
void missShader(inout visibilityRayPayload payload)
{
    payload.distance = -1.0f;
    payload.dstPatchId = 0xFFFFFFFF;
}