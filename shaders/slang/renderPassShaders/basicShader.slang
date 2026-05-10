module basicShader;

__include "pc_helpers";

#include "../utils/helpers.slang"
#include "../utils/constants.slang"
#include "../utils/common_structs.slang"

[[vk::binding(0, 0)]]
uniform RaytracingAccelerationStructure topLevelAS;

[[vk::binding(1, 0)]]
[vk::image_format("rgba8")] RWTexture2D<float4> outputImage;

[[vk::binding(2, 0)]]
ConstantBuffer<CameraData> cameraData;

[[vk::binding(3, 0)]]
uniform Sampler2D colorSampler[1];

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
StructuredBuffer<float4> radiosityLightmapBuffer;

[[vk::binding(11, 0)]]
StructuredBuffer<float3> vertexRadiosityBuffer;

[[vk::binding(12, 0)]]
StructuredBuffer<uint32_t> triToMaterialIdxBuffer;

[[vk::push_constant]] internal pc_blas blasAddr;

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
    if (cameraData.shadowMode)
    {
        RayPayload payload;
        payload.hitValue = float3(1.0, 1.0, 1.0);
        payload.shadowed = false;
        TraceRay(topLevelAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray, payload);
        float3 shadowColor = payload.shadowed ? shadowConst : float3(1.0, 1.0, 1.0);
        float3 finalColor = payload.hitValue * shadowColor;
        outputImage[dispatchIndex.xy] = float4(finalColor, 1.0);
        return;
    }
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

        float reflectivity = payload.reflector; // No clamp for metallic materials to achieve true mirrors
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

    if (cameraData.enableCUDA)
    {
        color = lerp(color, cudaColorBuffer[0].xyz, 0.5f);
        outputImage[dispatchIndex.xy] = float4(color, 1.0) + float4(cudaColorBuffer[0]) * 0.3f;
    } else
    {
        outputImage[dispatchIndex.xy] = float4(color, 1.0);
    }
}

[shader("miss")]
void missShader(inout RayPayload payload)
{
    // ========================== SHADOWS
    if (payload.shadowed)
    {
        payload.hitValue = float3(0.0, 0.0, 0.0);
        payload.shadowed = false;
        return;
    }
    // payload.shadowed = false;
    // payload.hitValue = float3(0.0, 0.0, 0.0);

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
    uint instanceIndex = InstanceIndex();
    uint triangleIndex = PrimitiveIndex();
    const GeometryInfo geometryInfo = geometryInfoBuffer[instanceIndex];
    uint triGlobalIndex = geometryInfo.triToPatchOffset + triangleIndex;
    uint triGlobalIndexForMat = geometryInfo.triToMaterialOffset + triangleIndex;
    uint patchId = triToPatchBuffer[triGlobalIndex];

    uint selectedPatchId = selectedPatchesBuffer[0].patchId;
    float unshotEnergy = selectedPatchesBuffer[0].unshotEnergy;

    uint matIdx = triToMaterialIdxBuffer[triGlobalIndexForMat];
    Material mat = materialBuffer[matIdx];
    uint32_t textureIndex = mat.textureIndex; // text

    Triangle tri = unpackTriangle(triangleIndex, attribs, geometryInfo);

    const uint globalVertexIdx0 = geometryInfo.vertexGlobalOffset + tri.vertexIndices[0];
    const uint globalVertexIdx1 = geometryInfo.vertexGlobalOffset + tri.vertexIndices[1];
    const uint globalVertexIdx2 = geometryInfo.vertexGlobalOffset + tri.vertexIndices[2];

    const float3 interpolatedVertexRadiosity =
        vertexRadiosityBuffer[globalVertexIdx0] * barycentricCoords.x +
        vertexRadiosityBuffer[globalVertexIdx1] * barycentricCoords.y +
        vertexRadiosityBuffer[globalVertexIdx2] * barycentricCoords.z;

    float4 color = float4(mat.albedo.rgb, 1.0f);

    // jezeli textureIndex == MAX_TEXTURES to znaczy ze obiekt nie ma tekstury
    // ============================= TEKSTURY ==================
    // if (textureIndex < MAX_TEXTURES)
    // {
    //     color = colorSampler[textureIndex].SampleLevel(tri.uv, 0.0);
    // }



    // // payload.hitValue = color.bgr; // Color from texture
    // // payload.hitValue = tri.vertices[PrimitiveIndex()].normal; // Color based on normal
    // payload.hitValue = materialBuffer[instanceIndex].albedo.bgr; // Color from material buffer

    if(mat.type == MaterialType::LIGHT)
    {
        payload.hitValue = color.bgr; // Color from material buffer
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0f;
        return;
    }
    else if(mat.type == MaterialType::METALLIC)
    {
        payload.hitValue = float3(1.0,0.0,0.0); // Color from material buffer
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.95f; // High reflectivity for metallic materials
        return;
    }
    else if (mat.type == MaterialType::NONE)
    {
        payload.hitValue = float3(0.0,0.0,0.0); // Color from material buffer
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0f;
        return;
    }

    if(cameraData.enableRadiosityPass && patchId == selectedPatchId)
    {
        color = float4(1.0, 0.0, 0.0, 1.0);
        payload.hitValue = color.bgr;
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0f;
        return;
    }

    if(cameraData.debugNormals)
    {
        payload.hitValue = tri.normal * 0.5 + 0.5; // mapowanie z [-1,1] na [0,1]
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0f;
        return;
    }

    if(cameraData.debugCenters)
    {
        payload.hitValue = patchBuffer[patchId].center; // Color based on patch center
        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0f;
        return;
    }

    if (cameraData.debugPatches)
    {
        float3 c = patchColor(patchId);
        payload.hitValue = c;

        payload.distance = RayTCurrent();
        payload.normal = tri.normal;
        payload.reflector = 0.0;
        return;
    }
    
    if(cameraData.shadowMode)
    {
        // float3 normal = tri.normal;
        float3 normal = normalize(
          mul((float3x3)ObjectToWorld3x4(), tri.normal)
        );
        float3 hitPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float3 toLight = cameraData.light_pos - hitPosition;
        float lightDistance = length(toLight);
        float3 lightVector = toLight / max(lightDistance, 1e-4);
        float dot_product = max(dot(lightVector, normal), 0.2);

        float3 baseColor = color.bgr;
        float3 shadedColor = baseColor * dot_product;

        RayDesc rayDesc;
        rayDesc.Origin = hitPosition + normal * 0.01;
        rayDesc.Direction = lightVector;
        rayDesc.TMin = 0.001;
        rayDesc.TMax = max(lightDistance - 0.02, 0.001);

        payload.shadowed = true;
        TraceRay(topLevelAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 0, rayDesc, payload);
        if (payload.shadowed)
        {
            payload.hitValue = shadedColor * shadowConst;
        }
        else
        {
            payload.hitValue = shadedColor;
        }
        return;
    }


    // TEST
    bool isMetallic = mat.type == MaterialType::METALLIC;
    if (isMetallic)
    {
        color = float4(materialBuffer[instanceIndex].albedo.rgb, 1.0);
    }

    // ========================== REFLECTIONS
    float3 normal = normalize(mul((float3x3)ObjectToWorld3x4(), tri.normal));
    if (dot(normal, WorldRayDirection()) > 0.0f)
    {
        normal = -normal;
    }
    float3 hitPosition = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 toLight = cameraData.light_pos - hitPosition;
    float lightDistance = length(toLight);
    float3 lightVector = toLight / max(lightDistance, 1e-4);
    float dot_product = max(dot(lightVector, normal), 0.0f);
    
    float3 FinalColor;
    
    if (isMetallic)
    {
        // For metallic materials: use specular reflection based on metallic albedo
        // The actual reflection is computed in rayGenShader via reflected rays
        float3 specularColor = color.rgb;
        float3 directLighting = specularColor * dot_product;
        // Metallic materials do not receive ambient or radiosity contributions
        FinalColor = directLighting;
    }
    else
    {
        // For diffuse materials: standard Lambertian shading with ambient and radiosity
        float3 directLighting = (color * dot_product).rgb;
        float3 ambientFloor = color.rgb * 0.03f;
        FinalColor = 0.8f * directLighting + ambientFloor;

        if (cameraData.useRadiosityLightmap)
        {
            const float indirectStrength = 0.4f;
            float3 indirect = interpolatedVertexRadiosity;
            if (dot(indirect, indirect) < 1e-10f)
            {
                // Fallback for seam/island vertices that have no valid adjacency in vertexRadiosityBuffer.
                indirect = radiosityLightmapBuffer[patchId].xyz;
            }

            indirect *= indirectStrength;
            FinalColor += indirect;

            // To jest po to, ze jak uzywam wyzsze wartosci emission > 1.0
            // To musze znormalizowac tą wartosc do przedzialu 0-1 zeby patche nie byly biale
            // To sie nazywa tone mapping
            FinalColor = FinalColor / (FinalColor + float3(1.0f));
        }
    }

    payload.hitValue = FinalColor;
    payload.distance = RayTCurrent();
    payload.normal = normal;
    // Set high reflectivity for metallic to achieve mirror-like appearance
    payload.reflector = isMetallic ? 0.95f : 0.0f;
}