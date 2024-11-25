#version 440 core

#define pi 3.1415926535897932384626433832795

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

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

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
layout(location = 3) uniform float dist;
layout(location = 4) uniform float angle;
layout(location = 5) uniform float turnAngle;

float hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x / 4294967295.0;
}

void move(inout Ant antCurr, uint curr) {
    bool xOutOfBounds = antCurr.pos.x < 0 || antCurr.pos.x > texSize.x;
    bool yOutOfBounds = antCurr.pos.y < 0 || antCurr.pos.y > texSize.y;
    antCurr.pos.x -= texSize.x * sign(antCurr.pos.x) * float(xOutOfBounds);
    antCurr.pos.y -= texSize.y * sign(antCurr.pos.y) * float(yOutOfBounds);

    // Calculating the cos and sin values ahead of the ant
    float cosL = cos(antCurr.dir + angle);
    float cosR = cos(antCurr.dir - angle);
    float sinL = sin(antCurr.dir + angle);
    float sinR = sin(antCurr.dir - angle);

    float frontPosX = antCurr.pos.x + cos(antCurr.dir) * dist;
    float frontPosY = antCurr.pos.y + sin(antCurr.dir) * dist;
    xOutOfBounds = frontPosX < 0 || frontPosX > texSize.x;
    yOutOfBounds = frontPosY < 0 || frontPosY > texSize.y;
    frontPosX -= texSize.x * sign(frontPosX) * float(xOutOfBounds);
    frontPosY -= texSize.y * sign(frontPosY) * float(yOutOfBounds);
    float lookFront = length(antImage[int(frontPosY) * texSize.x + int(frontPosX)].uPrev);

    float leftPosX = antCurr.pos.x + cosL * dist;
    float leftPosY = antCurr.pos.y + sinL * dist;
    xOutOfBounds = leftPosX < 0 || leftPosX > texSize.x;
    yOutOfBounds = leftPosY < 0 || leftPosY > texSize.y;
    leftPosX -= texSize.x * sign(leftPosX) * float(xOutOfBounds);
    leftPosY -= texSize.y * sign(leftPosY) * float(yOutOfBounds);
    float lookLeft = length(antImage[int(leftPosY) * texSize.x + int(leftPosX)].uPrev);

    float rightPosX = antCurr.pos.x + cosR * dist;
    float rightPosY = antCurr.pos.y + sinR * dist;
    xOutOfBounds = rightPosX < 0 || rightPosX > texSize.x;
    yOutOfBounds = rightPosY < 0 || rightPosY > texSize.y;
    rightPosX -= texSize.x * sign(rightPosX) * float(xOutOfBounds);
    rightPosY -= texSize.y * sign(rightPosY) * float(yOutOfBounds);
    float lookRight = length(antImage[int(rightPosY) * texSize.x + int(rightPosX)].uPrev);

    bool straight = lookFront >= lookLeft && lookFront >= lookRight;
    bool random = lookLeft >= lookFront && lookRight >= lookFront && !straight;
    bool left = lookLeft > lookFront && lookFront > lookRight && !random || random && (hash(curr * t) > 0.5);
    bool right = lookRight > lookFront && lookFront > lookLeft && !random || random && (hash(curr * t) <= 0.5);
    antCurr.dir += -turnAngle * (float(int(right)) - float(int(left)));
    antCurr.pos += (vel * vec2(cos(antCurr.dir), sin(antCurr.dir)));

    bool up = antCurr.dir >= pi;
    bool down = antCurr.dir <= pi;
    antCurr.color.x = float(up);
    antCurr.color.y = float(down);
    // antCurr.color.z = float(int(antCurr.dir * 10)) / (10 * 2 * pi);
}

void main() {
    uint curr = gl_GlobalInvocationID.x;

    move(ant[curr], curr);
    int texelCoord = int(ant[curr].pos.y) * int(texSize.x) + int(ant[curr].pos.x);
    antImage[texelCoord].u = ant[curr].color;
    // antImage[texelCoord].u = vec4(ant[curr].color);
}
