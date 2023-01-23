#version 330 core

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler2D u_mapTex;
uniform sampler2D u_bgTex;
uniform sampler2D u_circleTex;

uniform int u_mode;

void main() {

    // DEBUG: coordinates as color
    //vec3 color = v_ndcPos * 0.5 + 0.5;
    //vec3 color = v_coord3d;
    
    vec4 rawTexVal = texture( u_mapTex, v_coord3d.xy );
    vec3 color = rawTexVal.rgb;
    //vec3 color = texture( u_mapTex, vec2( v_coord3d.x, 0.5 ) ).rgb;

    if ( u_mode >= 1 ) { // also bg tex (for example for overlaying transparencies over histo tex)
        float bgVal = texture( u_bgTex, v_coord3d.xy ).r;
        vec3 bgCol = vec3( bgVal );
        color.rgb = mix(bgCol, vec3(color.r), color.g);
        //color.rgb = mix(vec3(0.3), vec3(0.8), color.g);
    }

    o_fragColor.rgb = color.rgb;
    //o_fragColor.a = 1.0;
    o_fragColor.a = rawTexVal.a;
}
