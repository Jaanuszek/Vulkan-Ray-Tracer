struct Attribs
{
    float2 barycentric;
};

struct Payload
{
    [[vk::location(0)]] float3 hitValue;
    // [[vk::location(1)]] float2 texCoord;
};

struct Vertex {
    float3 pos;
    float2 uv;
};

// struct IndexData {
//     uint indices[1]; // Dummy size
// };

// struct VertexData {
//     Vertex vertices[1]; // Dummy size
// };

struct Vertices {
    float4 v[1];
};

struct Indices {
    uint i[1];
};

struct BufferRefs {
    vk::BufferPointer<Vertices> vertexBuffer;
    vk::BufferPointer<Indices>  indexBuffer;
};

[[vk::push_constant]] BufferRefs bufferRefs;

struct Triangle {
    Vertex vertices[3];
    float2 uv;
};

[[vk::combinedImageSampler]]
Texture2D tex : register(t3);
[[vk::combinedImageSampler]][[vk::binding(3)]]
SamplerState viking_sampler : register(s3); //s# - sampler (SAMPLER)

Triangle unpacTriangle(uint index, int vertexSize, float3 bary)
{
    Triangle tri;
    const uint triIndex = index * 3;

    Indices indices = bufferRefs.indexBuffer.Get();
    Vertices vertices = bufferRefs.vertexBuffer.Get();

    for (uint i = 0; i < 3; i++)
    {
        const uint offset = indices.i[triIndex + i] * (vertexSize / 16);
        float4 d0 = vertices.v[offset + 0];
        float4 d1 = vertices.v[offset + 1];
        tri.vertices[i].pos = d0.xyz;
        tri.vertices[i].uv = d1.xy;
    }

    tri.uv = tri.vertices[0].uv * bary.x +
              tri.vertices[1].uv * bary.y +
              tri.vertices[2].uv * bary.z;
    return tri;
}

[shader("closesthit")]
void main(inout Payload p, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 bary = float3(
        1.0 - attr.barycentrics.x - attr.barycentrics.y,
        attr.barycentrics.x,
        attr.barycentrics.y
    );

    Triangle tri = unpacTriangle(PrimitiveIndex(), 32, bary);
    p.hitValue = float3(tri.uv, 0.0);


    p.hitValue = tex.SampleLevel(viking_sampler, float2(0.5, 0.0), 0.0).rgb;
    // float4 color = tex.SampleLevel(viking_sampler, float2(0.5,0.5), 0.0);
    // p.hitValue *= color.rgb;
}