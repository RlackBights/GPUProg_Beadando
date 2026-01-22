#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D heightMap;
layout(rg32f, binding = 1) writeonly uniform image2D minMaxOut;

shared float localMin[256];
shared float localMax[256];

const float INF = 3.4028235e38;

void main()
{
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    ivec2 lid = ivec2(gl_LocalInvocationID.xy);
    int idx = lid.y * 16 + lid.x;

    ivec2 size = textureSize(heightMap, 0);

    float h = 0.0;
    if (gid.x < size.x && gid.y < size.y)
        h = abs(texelFetch(heightMap, gid, 0).r);

    localMin[idx] = (gid.x < size.x && gid.y < size.y) ? h : INF;
    localMax[idx] = (gid.x < size.x && gid.y < size.y) ? h : -INF;

    barrier();

    for (int stride = 128; stride > 0; stride >>= 1) {
        if (idx < stride) {
            localMin[idx] = min(localMin[idx], localMin[idx + stride]);
            localMax[idx] = max(localMax[idx], localMax[idx + stride]);
        }
        barrier();
    }

    if (idx == 0 && gl_WorkGroupID.x == 0 && gl_WorkGroupID.y == 0) {
        imageStore(minMaxOut, ivec2(0, 0),
                   vec4(localMin[0], localMax[0], 0, 0));
    }
}
