#version 330 core
#extension GL_GOOGLE_include_directive : enable

layout ( location = 0 ) in vec3 a_pos;
layout ( location = 1 ) in vec3 a_norm;

uniform sampler3D u_densityLoResTex;
uniform sampler2D u_emptySpaceTex;

uniform mat4 u_mvpMat;
uniform vec3 u_volDimRatio;
uniform vec3 u_volOffset;

out vec3 v_coord3d;

#include "dvrCommonDefines.h.glsl"

#ifndef USE_EMPTY_SPACE_SKIPPING
    #define USE_EMPTY_SPACE_SKIPPING    1
#endif


void main() {
    //vec3 scaledPos = a_pos * u_volDimRatio;
    //vec3 scaledPos = a_pos * u_volDimRatio + u_volOffset;
    vec3 a_pos01 = ( a_pos * 0.5 + 0.5 );
    vec3 scaledPos = clamp( a_pos01 * u_volDimRatio + u_volOffset, 0.0, 1.0 ) * 2.0 - 1.0;
//    scaledPos = clamp(scaledPos, -1.0, 1.0);

    //vec3 scaledPos = a_pos;
    v_coord3d.xyz = scaledPos.xyz * 0.5 + 0.5;

#if ( USE_EMPTY_SPACE_SKIPPING != 0 )
//    ivec3 loResDim = textureSize( u_densityLoResTex, 0 );
//    vec3 fLoResDim = vec3( loResDim );
//    vec3 fRecipLoResDim = 1.0 / fLoResDim;
//
    //vec3 loResTexCoord = v_coord3d.xyz;// + 0.5 * fRecipLoResDim;
    vec3 loResTexCoord = u_volOffset + 0.5 * u_volDimRatio;

    vec2 minMaxLoRes = texture( u_densityLoResTex, loResTexCoord ).rg;
    minMaxLoRes *= HOUNSFIELD_UNIT_SCALE;

    float emptySpaceTableEntry = texture( u_emptySpaceTex, minMaxLoRes ).r;
    if ( emptySpaceTableEntry > 0.0 ) { // skip this brick
        scaledPos = vec3(-100.0);
        gl_Position = vec4( scaledPos, scaledPos.x );
        return;
    }
#endif

    gl_Position = u_mvpMat * vec4( scaledPos, 1.0 );

}