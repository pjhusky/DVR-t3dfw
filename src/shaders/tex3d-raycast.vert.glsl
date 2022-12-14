#version 330 core

layout (location = 0) in vec3 a_pos;

out vec3 v_ndcPos;
out vec3 v_coord3d;

void main() {
    v_ndcPos = a_pos.xyz;
    v_coord3d.xyz = a_pos.xyz * 0.5 + 0.5;

    gl_Position = vec4( a_pos, 1.0 );
}