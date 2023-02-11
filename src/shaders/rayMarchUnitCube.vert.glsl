#version 330 core

layout ( location = 0 ) in vec3 a_pos;
layout ( location = 1 ) in vec3 a_norm;

uniform mat4 u_mvpMat;

out vec3 v_coord3d;
 

void main() {
    gl_Position = u_mvpMat * vec4( a_pos, 1.0 );
    v_coord3d.xyz = a_pos.xyz * 0.5 + 0.5;
}