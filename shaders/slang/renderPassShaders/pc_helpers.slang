implementing basicShader;

public func unpackTriangle(uint idx, uint objIdx, HitAttribute attribs) -> Triangle
{
    Triangle tri;

    // kazdy trojkat ma 3 wierzcholki, wiec kazdy trojkat to wieloktrontosc 3 wierzcholkow
    const uint triIdx = idx * 3;
    const uint vertexSize = sizeof(Vertex);

    // iteracja po 3 kolejnych wierzchołkach
    for (uint i = 0; i < 3; i++)
    {
        const uint localVertexIdx = triIdx + i;
        const uint vertexIdx = ((uint*)geometryInfoBuffer[objIdx].indicesAddress)[localVertexIdx];
        const uint64_t vertexOffset = geometryInfoBuffer[objIdx].verticesAddress + vertexIdx * vertexSize;
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