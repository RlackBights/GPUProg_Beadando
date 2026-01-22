#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D divergenceTex;
layout(binding = 1) uniform sampler2D inputHeightMap;
layout(r32f, binding = 2) writeonly uniform image2D outputHeightMap;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = textureSize(inputHeightMap, 0);

    int xL = (coord.x > 0) ? coord.x - 1 : 1;
    int xR = (coord.x < size.x - 1) ? coord.x + 1 : size.x - 2;
    int yD = (coord.y > 0) ? coord.y - 1 : 1;
    int yU = (coord.y < size.y - 1) ? coord.y + 1 : size.y - 2;

    float left  = texelFetch(inputHeightMap, ivec2(xL, coord.y), 0).r;
    float right = texelFetch(inputHeightMap, ivec2(xR, coord.y), 0).r;
    float down  = texelFetch(inputHeightMap, ivec2(coord.x, yD), 0).r;
    float up    = texelFetch(inputHeightMap, ivec2(coord.x, yU), 0).r;

    float divergence = texelFetch(divergenceTex, coord, 0).r;

    float newHeight = 0.25 * (left + right + up + down - divergence);

    if (coord.x == 0 && coord.y == 0)
        newHeight = 0.0;

    imageStore(outputHeightMap, coord, vec4(newHeight, 0.0, 0.0, 0.0));
}
