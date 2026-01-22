#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;

layout(binding = 2, set = 0) uniform CameraMatrices {
    mat4 viewInverse;
    mat4 projInverse;
} matrices;


layout(location = 0) rayPayloadEXT vec3 hitValue;

void main() 
{
    const uvec3 launchIndex = gl_LaunchIDEXT;
    const uvec3 launchDim = gl_LaunchSizeEXT;
    
    const vec2 pixelCenter = vec2(launchIndex.xy) + vec2(0.5);
    const vec2 UV = pixelCenter / vec2(launchDim.xy);
    
    vec2 NDC = UV * 2.0 - 1.0;
    vec4 target = matrices.projInverse * vec4(NDC.x, NDC.y, 1.0, 1.0);
    
    vec3 origin = (matrices.viewInverse * vec4(0, 0, 0, 1)).xyz;
    vec3 direction = (matrices.viewInverse * vec4(normalize(target.xyz), 0)).xyz;
    
    float tMin = 0.001;
    float tMax = 10000.0;
    
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin, tMin, direction, tMax, 0);
    
    imageStore(image, ivec2(launchIndex.xy), vec4(hitValue, 0.0));
}
