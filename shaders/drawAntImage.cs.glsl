#version 440 core

struct Ant {
    vec2 pos;
    float dir;
    float u;
    vec4 color;
};

struct Image {
    vec4 u;
    vec4 uPrev;
};

layout(rgba32f) uniform image2D imgOutput;

layout(binding = 0, std430) buffer Ants {
    Ant ant[];
};
layout(binding = 1, std430) buffer AntImage {
    Image antImage[];
};

layout(location = 0) uniform uint t;
layout(location = 1) uniform uvec2 texSize;
layout(location = 2) uniform float vel;
layout(location = 3) uniform uint stage;

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

float hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x / 4294967295.0;
}

void main() {
    uint curr = gl_GlobalInvocationID.x;
    antImage[curr].uPrev = antImage[curr].u;
    vec4 value = vec4(0.0, 0.0, 0.0, 1.0);
    uint j = uint(floor(gl_GlobalInvocationID.x / texSize.x));
    uint i = gl_GlobalInvocationID.x % texSize.x;
    ivec2 texelCoord = ivec2(i, j);

    antImage[curr].u = 0.95 * (antImage[curr].uPrev + 0.25 * (antImage[curr - 1].uPrev + antImage[curr + 1].uPrev + antImage[curr + texSize.x].uPrev + antImage[curr - texSize.x].uPrev - 4 * antImage[curr].uPrev));

    bool isZero = length(antImage[curr].u) > 1e-2;
    value = antImage[curr].u * vec4(isZero);

    imageStore(imgOutput, texelCoord, value);
}
