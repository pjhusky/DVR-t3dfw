#version 330 core

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;


void main() {

    vec3 color = v_ndcPos * 0.5 + 0.5;

    o_fragColor.rgb = color.rgb;
    o_fragColor.a = 1.0;
}
