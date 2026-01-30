#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0) uniform sampler2D normalTex;
layout(r32f, binding = 1) writeonly uniform image2D divergenceTex;

// void main() {
//     ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
//     ivec2 size = textureSize(normalTex, 0);
//     if (any(greaterThanEqual(coord, size))) return;

//     ivec2 cL = clamp(coord + ivec2(-1,0), ivec2(0), size-1);
//     ivec2 cR = clamp(coord + ivec2( 1,0), ivec2(0), size-1);
//     ivec2 cU = clamp(coord + ivec2(0, 1), ivec2(0), size-1);
//     ivec2 cD = clamp(coord + ivec2(0,-1), ivec2(0), size-1);

//     vec3 nC = normalize(texelFetch(normalTex, coord, 0).rgb * 2.0 - 1.0);
//     vec3 nL = normalize(texelFetch(normalTex, cL, 0).rgb * 2.0 - 1.0);
//     vec3 nR = normalize(texelFetch(normalTex, cR, 0).rgb * 2.0 - 1.0);
//     vec3 nU = normalize(texelFetch(normalTex, cU, 0).rgb * 2.0 - 1.0);
//     vec3 nD = normalize(texelFetch(normalTex, cD, 0).rgb * 2.0 - 1.0);

//     float eps = 0.2;
//     float pR = -nR.x / max(nR.z, eps);
//     float pL = -nL.x / max(nL.z, eps);
//     float qU = -nU.y / max(nU.z, eps);
//     float qD = -nD.y / max(nD.z, eps);

//     float divergence = 0.5 * ((pR - pL) + (qU - qD));
//     if (coord.x == 0 || coord.x == size.x-1 || coord.y == 0 || coord.y == size.y-1) { // Genuinely no idea hogy miért volt ennyire béna de ig ez megoldja
//         divergence = 0.0;
//     }

//     imageStore(divergenceTex, coord, vec4(divergence, 0,0,1));
// }

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = textureSize(normalTex, 0);
    if (any(greaterThanEqual(coord, size))) return;

    ivec2 cL = clamp(coord + ivec2(-1,0), ivec2(0), size-1);
    ivec2 cR = clamp(coord + ivec2( 1,0), ivec2(0), size-1);
    ivec2 cU = clamp(coord + ivec2(0, 1), ivec2(0), size-1);
    ivec2 cD = clamp(coord + ivec2(0,-1), ivec2(0), size-1);

    vec3 nC = normalize(texelFetch(normalTex, coord, 0).rgb * 2.0 - 1.0);
    vec3 nL = normalize(texelFetch(normalTex, cL, 0).rgb * 2.0 - 1.0);
    vec3 nR = normalize(texelFetch(normalTex, cR, 0).rgb * 2.0 - 1.0);
    vec3 nU = normalize(texelFetch(normalTex, cU, 0).rgb * 2.0 - 1.0);
    vec3 nD = normalize(texelFetch(normalTex, cD, 0).rgb * 2.0 - 1.0);

    float eps = 0.2;
    float pR = -nR.x / max(nR.z, eps);
    float pL = -nL.x / max(nL.z, eps);
    float qU = -nU.y / max(nU.z, eps);
    float qD = -nD.y / max(nD.z, eps);

    float divergence = 0.5 * ((pR - pL) + (qU - qD));
    if (coord.x == 0 || coord.x == size.x-1 || coord.y == 0 || coord.y == size.y-1) { // Genuinely no idea hogy miért volt ennyire béna de ig ez megoldja
        divergence = 0.0;
    }

    imageStore(divergenceTex, coord, vec4(divergence, 0,0,1));
}