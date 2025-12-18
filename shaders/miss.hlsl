struct Payload
{
    [[vk::location(0)]] float3 hitValue;
};

[shader("miss")]
void main(inout Payload payload)
{
    payload.hitValue = float3(1.0, 1.0, 1.0); // kolor tła - czarny
}