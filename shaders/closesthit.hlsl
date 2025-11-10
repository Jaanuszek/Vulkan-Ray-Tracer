struct Attribs
{
    float2 barycentric;
};

struct Payload
{
    [[vk::location(0)]] float3 hitValue;
};

// in Attribs jest obliczane przez vulkan
// współrzędne barycentryczne to taka interpolacja jak w fragment shaderze, tylko że ręcznie
[shader("closesthit")]
void main(inout Payload p, in BuiltInTriangleIntersectionAttributes attr)
{
    // Współrzędne barycentryczne to jakby waga każdego z wierzchołków trójkąta
    // Mówią w jakich proporcjach danego trójkąta trafiliśmy promieniem
    // znając dwie współrzędne barycentryczne mozna latwo obliczyc trzecią, bo
    // b1 + b2 + b3 = 1
    // gdzie b1, - współrzędna barycentryczna wierzchołka 1
    //       b2, - współrzędna barycentryczna wierzchoł
    //       b3, - współrzędna barycentryczna wierzchołka 3
    // P = b1*V1 + b2*V2 + b3*V3
    // gdzie P - punkt przecięcia promienia z trójkątem
    //       V1, V2, V3 - wierzchołki trójkąta
    //       b1, b2, b3 - współrzędne barycentryczne

    float3 barycentricCoords = float3(
        1.0 - attr.barycentrics.x - attr.barycentrics.y,
        attr.barycentrics.x,
        attr.barycentrics.y
    );

    p.hitValue = barycentricCoords;
}