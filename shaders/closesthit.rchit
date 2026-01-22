#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

struct Vertex {
    vec3 pos;
    vec2 uv;
};

struct Triangle {
	Vertex vertices[3];
	vec2 uv;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer IndexBuffer {
    uint i[];
};

layout(binding = 3, set = 0) uniform sampler2D textureSampler;

layout(push_constant) uniform PushConstants {
    uint64_t vertexBuffer;
    uint64_t indexBuffer;
} pushConstants;

Triangle unpackTriangle(uint index) {
	Triangle tri;
	const uint triIndex = index * 3;

	IndexBuffer  indices   = IndexBuffer(pushConstants.indexBuffer);
	VertexBuffer vertexBuf = VertexBuffer(pushConstants.vertexBuffer);

	// Unpack vertices - directly access VertexRT structure
	for (uint i = 0; i < 3; i++) {
		const uint vertexIdx = indices.i[triIndex + i];
		Vertex v = vertexBuf.vertices[vertexIdx];
		tri.vertices[i].pos = v.pos;
		tri.vertices[i].uv = v.uv;
	}
	// Calculate values at barycentric coordinates
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
	return tri;
}

void main()
{
    // Check if buffer addresses are valid
    if (pushConstants.vertexBuffer == 0 || pushConstants.indexBuffer == 0)
    {
        hitValue = vec3(1.0, 0.0, 0.0); // red = invalid addresses
        return;
    }
    
    Triangle tri = unpackTriangle(gl_PrimitiveID);
    vec4 color = texture(textureSampler, tri.uv);
    // Swap R and B channels if format mismatch
    hitValue = color.rgb; // Try swapping channels
}
