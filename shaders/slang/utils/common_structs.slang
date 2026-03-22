public struct CameraData
{
    bool enableCUDA;
    bool shadowMode;
    bool debugPatches;
    bool debugNormals;
    bool debugCenters;
    bool useRadiosityLightmap;
    bool enableRadiosityPass;
    float3 light_pos;
    column_major float4x4 viewInverse;
    column_major float4x4 projInverse;
};

public struct Patch
{
    uint32_t id;
    float area;
    float emission = 0.0f;
    float3 center;
    float3 normal;
    float3 albedo;
    // float unshotEnergy = 0.0f;
    float unshotEnergy = 0.0f;
    float3 radiosity = float3(0.0f, 0.0f, 0.0f);
};

public struct SelectedPatch
{
    uint32_t patchId;
    float unshotEnergy;
};

public struct RayPayload
{
    float3 hitValue;
    float distance;
    float3 normal;
    float reflector;
    // ========================== SHADOWS
    bool shadowed;
};

public struct visibilityRayPayload
{
    float distance;
    uint32_t dstPatchId;
};

public struct HitAttribute
{
    float2 bary;
};

public struct GeometryInfo
{
    uint32_t triToPatchOffset;
    uint32_t triangleCount;
    uint64_t verticesAddress;
    uint64_t indicesAddress;
};

public enum class MaterialType
{
    NONE = 0,
    ALBEDO = 1,
    METALLIC = 2,
    ROUGHNESS = 3,
    PBR = 4,
    LIGHT = 5
};

public struct Material
{
    float4 albedo;
    float metallic; // 4B
    float roughness; // 4B
    MaterialType type; // 4B
    uint32_t textureIndex;
};


public struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;
};

public struct Triangle
{
    Vertex[3] vertices;
    float3 normal;
    float2 uv;
}

public struct pc_blas
{
    uint64_t verticesAddress;
    uint64_t indicesAddress;
};

public struct PatchVisibility
{
    uint32_t srcPatchId;
    uint32_t dstPatchId;
    float visibility;
}

public func unpackTriangle(uint idx, HitAttribute attribs, GeometryInfo geometryInfo) -> Triangle
{
    Triangle tri;

    // kazdy trojkat ma 3 wierzcholki, wiec kazdy trojkat to wieloktrontosc 3 wierzcholkow
    const uint triIdx = idx * 3;
    const uint vertexSize = sizeof(Vertex);

    // iteracja po 3 kolejnych wierzchołkach
    for (uint i = 0; i < 3; i++)
    {
        const uint localVertexIdx = triIdx + i;
        const uint vertexIdx = ((uint*)geometryInfo.indicesAddress)[localVertexIdx];
        const uint64_t vertexOffset = geometryInfo.verticesAddress + vertexIdx * vertexSize;
        Vertex* v = (Vertex*)vertexOffset;
        tri.vertices[i].pos = v->pos;
        tri.vertices[i].normal = v->normal;
        tri.vertices[i].uv = v->uv;
    }
    
    float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    tri.normal = normalize(tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z);
    return tri;
}