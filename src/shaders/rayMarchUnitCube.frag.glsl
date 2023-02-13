#version 330 core
#extension GL_GOOGLE_include_directive : enable

in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;
uniform sampler3D u_gradientTex;
uniform sampler2D u_colorAndAlphaTex;

uniform vec3 u_minMaxScaleVal;
uniform vec4 u_camPos_OS;
uniform vec3 u_volDimRatio;

#include "shaderUtils.h.glsl"
#include "dvrCommonDefines.h.glsl"

void main() {

    float maxSteps = 300.0;
    vec4 color = vec4( 0.0 );
    float numPosDensities = 1.0;

    vec3 ray_end = v_coord3d;
    vec3 ray_start = u_camPos_OS.xyz * 0.5 + 0.5;
    vec3 ray_dir = ( ray_end - ray_start );
    float tnear, tfar;
	int hit = intersectBox(ray_start, ray_dir, vec3(-1.0), vec3(1.0), tnear, tfar);
    tnear = max( 0.0, tnear );
    vec3 curr_sample_pos = ray_start + tnear * ray_dir;

    vec3 ray_dir_step = ( ray_end - curr_sample_pos ) / maxSteps;
    
    for ( float currStep = 0.0; currStep < maxSteps; currStep += 1.0, curr_sample_pos += ray_dir_step ) {
        float texVal = texture( u_densityTex, curr_sample_pos ).r;
        color += vec4( texVal );
        numPosDensities += (texVal > 0.0) ? 0.25 : 0.0;
    }
    
    color = ( color - u_minMaxScaleVal.x ) * u_minMaxScaleVal.z; // map volume-tex values to 0-1 after the loop

    o_fragColor = color / numPosDensities;
    o_fragColor.a = 1.0;
}
