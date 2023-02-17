#version 330 core
#extension GL_GOOGLE_include_directive : enable

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;
uniform sampler3D u_gradientTex;
uniform sampler2D u_colorAndAlphaTex;

// L top
// L btm
// R btm
// R top
uniform vec4 u_fpDist_OS[4];

uniform vec2 u_surfaceIsoAndThickness;

uniform vec3 u_minMaxScaleVal;
uniform vec4 u_camPos_OS;
uniform vec3 u_volDimRatio;
//uniform float u_recipTexDim;

uniform int u_frameNum;

#include "shaderUtils.h.glsl"
#include "dvrCommonDefines.h.glsl"

#ifndef DVR_MODE
    #define DVR_MODE                LEVOY_ISO_SURFACE
    //#define DVR_MODE                F2B_COMPOSITE
    //#define DVR_MODE                XRAY
    //#define DVR_MODE                MRI
#endif

void main() {
    vec4 fp_x_ipol_top = mix( u_fpDist_OS[ 0 ], u_fpDist_OS[ 3 ], v_coord3d.x );
    vec4 fp_x_ipol_btm = mix( u_fpDist_OS[ 1 ], u_fpDist_OS[ 2 ], v_coord3d.x );
    vec4 fp_xy_ipol = mix(fp_x_ipol_btm, fp_x_ipol_top, v_coord3d.y);

    // project interpolated homogeneous OS pos to real OS
    vec3 ray_far_OS = fp_xy_ipol.xyz / fp_xy_ipol.w;

    vec3 ray_end   = ray_far_OS;
    vec3 ray_start = u_camPos_OS.xyz;
    vec3 ray_dir = ray_end - ray_start; // no need to normalize here

    float tnear, tfar;
    int hit = intersectBox(ray_start, ray_dir, -u_volDimRatio, u_volDimRatio, tnear, tfar);
    if ( hit == 0 ) { discard; return; }

    tnear = max( 0.0, tnear );
    tfar = max( tfar, tnear );
    vec3 curr_sample_pos = ray_start + tnear * ray_dir;
    vec3 end_sample_pos = ray_start + tfar * ray_dir;

    // perform transformation into "non-square" dataset only once instead of at each sampling position
    curr_sample_pos = curr_sample_pos / u_volDimRatio * 0.5 + 0.5;
    end_sample_pos = end_sample_pos / u_volDimRatio * 0.5 + 0.5;

    vec4 color = vec4( 0.0 );
#if ( DVR_MODE == XRAY )
    float numPosDensities = 0.0;
#endif
    
    //uvec2 pix = uvec2( uint( gl_FragCoord.x ), uint( gl_FragCoord.y ) );
    uvec2 pix = uvec2( uint( gl_FragCoord.x + 0.17 * u_frameNum ), uint( gl_FragCoord.y + 0.17 * u_frameNum ) );
    uvec3 randInput = uvec3(pix, pix.x*7u+pix.y*3u);
    vec3 rnd01 = rand01(randInput);


    float lenInVolume = length( end_sample_pos - curr_sample_pos );
    //o_fragColor.rgb = vec3(lenInVolume); o_fragColor.a = 1.0; return;

    vec3 vol_step_ray_unit = normalize( end_sample_pos - curr_sample_pos );
    
    vec3 vol_step_ray = vol_step_ray_unit * RAY_STEP_SIZE; // max steps roughly 300

    float surfaceIso = u_surfaceIsoAndThickness.x;
    float surfaceThickness = u_surfaceIsoAndThickness.y;

    vec3 curr_sample_pos_noRand = curr_sample_pos + 0.5 * vol_step_ray; // ensure that even with random -0.5 offset we are inside the volume
    int iStep = 0;
    for ( float currStep = 0.0; currStep < lenInVolume; currStep += RAY_STEP_SIZE, curr_sample_pos_noRand += vol_step_ray ) {

        curr_sample_pos = curr_sample_pos_noRand + vol_step_ray * ( rnd01[iStep] - 0.5 );
        iStep++; iStep %= 3;
        if ( iStep == 2 ) {
            rnd01 = rand01(randInput);
        }

        float raw_densityVal = texture( u_densityTex, curr_sample_pos ).r;
        raw_densityVal *= HOUNSFIELD_UNIT_SCALE;

        vec4 colorAndAlpha = texture( u_colorAndAlphaTex, vec2( raw_densityVal, 0.5 ) );
        vec3 currColor = colorAndAlpha.rgb;

    #if ( DVR_MODE == LEVOY_ISO_SURFACE )
        vec3 gradient = texture( u_gradientTex, curr_sample_pos ).rgb;
        float len_gradient = length( gradient );
        vec3 gradient_unit = gradient / len_gradient;

        float currAlpha = 0.0;
        if ( len_gradient <= 0.000000001 && raw_densityVal == surfaceIso ) { currAlpha = 1.0; }
        else if ( len_gradient > 0.0 && ( surfaceIso - surfaceThickness * len_gradient < raw_densityVal && raw_densityVal < surfaceIso + surfaceThickness * len_gradient ) ) {
            currAlpha = 1.0 - 1.0 / surfaceThickness * abs( ( surfaceIso - raw_densityVal ) / len_gradient );
        }
        currAlpha *= colorAndAlpha.a;

        float n_dot_l_raw = dot( lightDir.xyz, gradient_unit );
        float diffuseIntensity = diffuse * max( 0.0, n_dot_l_raw );
        float clampedSpecularIntensity = max( 0.0, ( dot( vol_step_ray_unit, reflect( gradient_unit, -lightDir.xyz ) ) ) );
        float specularIntensity = specular * ( ( n_dot_l_raw <= 0.0 ) ? 0.0 : clampedSpecularIntensity );
        currColor = (ambient + diffuseIntensity + specularIntensity) * currColor;

        float currBlendFactor = ( 1.0 - color.a ) * currAlpha;
        color.rgb = color.rgb + currBlendFactor * currColor;
        color.a   = color.a + currBlendFactor;
    #elif ( DVR_MODE == F2B_COMPOSITE )
        vec3 gradient = texture( u_gradientTex, curr_sample_pos ).rgb;
        vec3 gradient_unit = normalize( gradient );
        
        float currAlpha = colorAndAlpha.a;

        float n_dot_l_raw = dot( lightDir.xyz, gradient_unit );
        float diffuseIntensity = diffuse * max( 0.0, n_dot_l_raw );
        float clampedSpecularIntensity = max( 0.0, ( dot( vol_step_ray_unit, reflect( gradient_unit, -lightDir.xyz ) ) ) );
        float specularIntensity = specular * ( ( n_dot_l_raw <= 0.0 ) ? 0.0 : clampedSpecularIntensity );
        currColor = (ambient + diffuseIntensity + specularIntensity) * currColor;

        float currBlendFactor = ( 1.0 - color.a ) * currAlpha;
        color.rgb = color.rgb + currBlendFactor * currColor;
        color.a   = color.a + currBlendFactor;
    #elif ( DVR_MODE == XRAY )
        color += vec4( colorAndAlpha.rgb * colorAndAlpha.a, colorAndAlpha.a );
        numPosDensities += 1.0;
    #elif ( DVR_MODE == MRI )
        if ( colorAndAlpha.a > color.a ) {
            color.a = colorAndAlpha.a;
            color.rgb = colorAndAlpha.rgb;
        }
    #endif

    #if ( DVR_MODE != XRAY )
        if ( color.a >= 0.99 ) {
            break;
        }
    #endif

    }

    #if ( DVR_MODE == XRAY )
        color = ( color - u_minMaxScaleVal.x ) * u_minMaxScaleVal.z; // map volume-tex values to 0-1 after the loop
        o_fragColor.rgb = color.rgb / numPosDensities;
    #else
        o_fragColor.rgb = color.rgb * lightColor.rgb;
    #endif

    //o_fragColor.a = 1.0;
    o_fragColor.a = 0.25;
}

