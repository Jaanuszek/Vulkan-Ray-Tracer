implementing basicShader;

// func rand_unit_vec()->float3 {
//     return normalize(tan(hash3(seed)))
// }

func rand(float2 st)->float
{
    return fract(sin(dot(st.xy, float2(12.9898, 78.233))) * 43758.5453123);
}

func rand_unit_vec(float2 st)->float3
{
    float z = rand(st) * 2.0 - 1.0; // [-1,1]
    float r = sqrt(1.0 - z * z);
    float phi = rand(st.yx) * 6.28318530718; // [0, 2PI]
    return float3(r * cos(phi), r * sin(phi), z);
}

float3 patchColor(uint id)
{
    if (id == 0) return float3(1,0,0).bgr;
    if (id == 1) return float3(0,1,0).bgr;
    if (id == 2) return float3(0,0,1).bgr;
    float r = frac(sin(float(id) * 12.9898) * 43758.5453);
    float g = frac(sin(float(id) * 78.233)  * 24634.6345);
    float b = frac(sin(float(id) * 39.425)  * 56445.2345);
    return float3(r,g,b).bgr;
}