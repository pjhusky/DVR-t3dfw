#version 330 core

layout (location = 0) in vec3 a_pos;

uniform vec2 u_scaleOffset;

out vec3 v_ndcPos;
out vec3 v_coord3d;

void main() {
    v_ndcPos = a_pos.xyz;
    v_coord3d.xyz = a_pos.xyz * 0.5 + 0.5;

//    v_ndcPos.y *= 0.1;
//    v_ndcPos.y -= 0.9;

    v_ndcPos.y *= u_scaleOffset.x;
    v_ndcPos.y -= u_scaleOffset.y;

    //gl_Position = vec4( a_pos, 1.0 );
    gl_Position = vec4( v_ndcPos, 1.0 );
}