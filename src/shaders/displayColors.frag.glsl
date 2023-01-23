#version 330 core

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler2D u_mapTex;


void main() {

    //vec3 color = v_ndcPos * 0.5 + 0.5;
    //vec3 color = v_coord3d;
    
    //vec3 color = texture( u_mapTex, v_coord3d.xy ).rgb;
    vec3 color = texture( u_mapTex, vec2( v_coord3d.x, 0.5 ) ).rgb;

    o_fragColor.rgb = color.rgb;
    o_fragColor.a = 1.0;
}
