#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D jacobiHeightMap;
layout(binding = 1) uniform sampler2D minmax;
layout(r32f, binding = 2) writeonly uniform image2D finalHeightMap;

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    float newHeight = abs(texelFetch(jacobiHeightMap, coord, 0).r);
    float h_max = abs(texture(minmax, vec2(0, 0)).r);
    float h_min = abs(texture(minmax, vec2(0, 0)).g);
    // imageStore(finalHeightMap, coord, vec4(vec3((newHeight - h_min) / (h_max - h_min)), 1.0));

    imageStore(finalHeightMap, coord, vec4(vec3((1-newHeight-h_min)), 1.0));
}