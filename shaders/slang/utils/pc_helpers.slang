implementing basicShader;

public struct Vertex
{
    float3 pos;
    float2 uv;
};

public struct Triangle
{
    Vertex[3] vertices;
    float2 uv;
}

public struct pc_blas
{
    uint64_t verticesAddress;
    uint64_t indicesAddress;
};

public func unpackTriangle(uint idx, HitAttribute attribs) -> Triangle
{
    Triangle tri;

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
        uint* indexPtr = (uint*)indexAddress; 
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