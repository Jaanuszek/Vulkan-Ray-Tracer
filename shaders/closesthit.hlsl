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

struct IndexData {
    uint indices[1]; // Dummy size
};

struct VertexData {
    Vertex vertices[1]; // Dummy size
};

struct BufferRefs {
    vk::BufferPointer<VertexData> vertexBuffer;
    vk::BufferPointer<IndexData>  indexBuffer;
};

[[vk::push_constant]] BufferRefs bufferRefs;

struct Triangle {
    Vertex vertices[3];
    float2 uv;
};

[shader("closesthit")]
void main(inout Payload p, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 bary = float3(
        1.0 - attr.barycentrics.x - attr.barycentrics.y,
        attr.barycentrics.x,
        attr.barycentrics.y
    );

    uint prim = PrimitiveIndex();

    // Access via bufferRefs.indexBuffer.Get().indices[offset]
    uint i0 = bufferRefs.indexBuffer.Get().indices[prim * 3 + 0];
    uint i1 = bufferRefs.indexBuffer.Get().indices[prim * 3 + 1];
    uint i2 = bufferRefs.indexBuffer.Get().indices[prim * 3 + 2];

    Vertex v0 = bufferRefs.vertexBuffer.Get().vertices[i0];
    Vertex v1 = bufferRefs.vertexBuffer.Get().vertices[i1];
    Vertex v2 = bufferRefs.vertexBuffer.Get().vertices[i2];

    float3 hitPos = v0.pos * bary.x + v1.pos * bary.y + v2.pos * bary.z;
    p.hitValue = hitPos;
}