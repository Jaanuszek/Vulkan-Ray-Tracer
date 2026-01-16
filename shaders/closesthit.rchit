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

layout(buffer_reference, scalar) readonly buffer VertexBuffer {
    vec4 v[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer {
    uint i[];
};

layout(binding = 3, set = 0) uniform sampler2D textureSampler;

layout(push_constant) uniform PushConstants {
    uint64_t vertexBuffer;
    uint64_t indexBuffer;
} pushConstants;

Triangle unpackTriangle(uint index, int vertexSize) {
	Triangle tri;
	const uint triIndex = index * 3;

	IndexBuffer  indices   = IndexBuffer(pushConstants.indexBuffer);
	VertexBuffer vertices  = VertexBuffer(pushConstants.vertexBuffer);

	// Unpack vertices
	// Data is packed as vec4 so we can map to the glTF vertex structure from the host side
	for (uint i = 0; i < 3; i++) {
		const uint offset = indices.i[triIndex + i] * (vertexSize / 16);
		vec4 d0 = vertices.v[offset + 0]; // pos.xyz, n.x
		vec4 d1 = vertices.v[offset + 1]; // n.yz, uv.xy
		tri.vertices[i].pos = d0.xyz;
		tri.vertices[i].uv = d1.zw;
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
    
    Triangle tri = unpackTriangle(gl_PrimitiveID, 32);
    hitValue = vec3(tri.uv, 0.0f);
    // Fetch the color for this ray hit from the texture at the current uv coordinates
    vec4 color = texture(textureSampler, tri.uv);
    hitValue = color.rgb;
}
