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