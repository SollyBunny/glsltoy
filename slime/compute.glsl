#version 430 core
layout(local_size_x = 1, local_size_y = 1) in;
layout(binding = 0) buffer DataBuffer {
    vec2 data[];
};
void main() {
    uint id = gl_GlobalInvocationID.x;
    data[id].x += 1.0;
}