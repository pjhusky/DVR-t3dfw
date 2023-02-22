#version 330 core
#extension GL_GOOGLE_include_directive : enable

in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;
uniform sampler3D u_gradientTex;
uniform sampler3D u_densityLoResTex;
uniform sampler2D u_colorAndAlphaTex;

uniform vec2 u_surfaceIsoAndThickness;

uniform vec3 u_minMaxScaleVal;
uniform vec4 u_camPos_OS;

uniform vec3 u_volDimRatio;
uniform vec3 u_volOffset;

//uniform float u_recipTexDim; // not needed here

uniform int u_frameNum;

#include "shaderUtils.h.glsl"
#include "dvrCommonDefines.h.glsl"

#ifndef DVR_MODE
    #define DVR_MODE                LEVOY_ISO_SURFACE
    //#define DVR_MODE                F2B_COMPOSITE
    //#define DVR_MODE                XRAY
    //#define DVR_MODE                MRI
#endif

#ifndef USE_EMPTY_SPACE_SKIPPING
    #define USE_EMPTY_SPACE_SKIPPING    1
#endif

void main() {

    vec3 curr_sample_pos;



    vec3 ray_end = v_coord3d;
    //o_fragColor.rgb = ray_end; o_fragColor.a = 1.0; return;

    vec3 ray_start = u_camPos_OS.xyz * 0.5 + 0.5;
    vec3 ray_dir = ray_end - ray_start; // no need to normalize here
    float tnear, tfar;

#if ( USE_EMPTY_SPACE_SKIPPING != 0 )
    //ivec3 loResDim = textureSize( u_densityLoResTex, 0 );
    //vec3 fLoResDim = vec3( loResDim );
    //vec3 fRecipLoResDim = 1.0 / fLoResDim;
    //int hit = intersectBox(ray_start, ray_dir, u_volOffset, u_volOffset + u_volDimRatio * fRecipLoResDim, tnear, tfar);
    int hit = intersectBox(ray_start, ray_dir, u_volOffset, u_volOffset + u_volDimRatio, tnear, tfar);
    //if ( hit == 0 ) {o_fragColor.rgb = curr_sample_pos; o_fragColor.a = 1.0; return;}
    //if ( hit == 0 ) {o_fragColor.rgb = vec3( 1.0, 0.8, 0.0 ); o_fragColor.a = 1.0; return;}

    #if 1
        if ( tfar - tnear < 0.004 ) {

            //o_fragColor.rgb = vec3( 1.0 ); o_fragColor.a = 1.0; // <<< COOL !!! brick edge outlines overlayed over normal rendering !!!
        
    //        //tnear = max( 0.0, tnear );
    //        //curr_sample_pos = ray_start + tnear * ray_dir;
    //        curr_sample_pos = ray_end;
    //        float raw_densityVal = texture( u_densityTex, curr_sample_pos ).r;
    //        raw_densityVal *= HOUNSFIELD_UNIT_SCALE;
    //        vec4 colorAndAlpha = texture( u_colorAndAlphaTex, vec2( raw_densityVal, 0.5 ) );
    //        vec3 currColor = colorAndAlpha.rgb;
            //o_fragColor.rgb = currColor; o_fragColor.a = colorAndAlpha.a;
//            gl_FragDepth = 1.0;
//            o_fragColor.rgb = currColor; o_fragColor.a = 1.0;
            discard;
        
            return;
        }
    #endif
#else    
    int hit = intersectBox(ray_start, ray_dir, vec3(0.0), vec3(1.0), tnear, tfar);
#endif
    

    tnear = max( 0.0, tnear );
    curr_sample_pos = ray_start + tnear * ray_dir;
    
#if ( DEBUG_VIS_MODE == DEBUG_VIS_BRICKS )
    //o_fragColor.rgb = curr_sample_pos; o_fragColor.a = 1.0; return;
    ivec3 loResDim = textureSize( u_densityLoResTex, 0 );
    vec3 fLoResDim = vec3( loResDim );
    vec3 fRecipLoResDim = 1.0 / fLoResDim;

    //gl_FragDepth = 0.0; // consider this pixel done by writing 0 depth (nothing will be closer, thus nothing can draw "over" this pixel anymore
    //o_fragColor.rgb = u_volOffset + 0.5 * u_volDimRatio * fRecipLoResDim; o_fragColor.a = 1.0; return; // LOD coloring
    
    o_fragColor.rgb = curr_sample_pos; o_fragColor.a = 1.0; return; // smooth coloring
    //o_fragColor.rgb = curr_sample_pos; o_fragColor.a = 0.1; return; // smooth coloring
    
    //o_fragColor.rgb = ray_end; o_fragColor.a = 1.0; return; // smooth coloring
#endif


    vec3 end_sample_pos = ray_end;


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
            color.a = 1.0;
            gl_FragDepth = 1.0; // consider this pixel done by writing 0 depth (nothing will be closer, thus nothing can draw "over" this pixel anymore
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

    //gl_FragDepth = 1.0 - 1.0/length(ray_dir); //tnear;
    //gl_FragDepth = 0.5 * ( tnear + tfar ); //tnear;

    gl_FragDepth = ( color.a >= 0.99 ) ? 0.0 : 1.0;
    //gl_FragDepth = 1.0; //
    //gl_FragDepth = gl_FragCoord.z;
    o_fragColor.a = color.a;

    //o_fragColor.a = 0.25;
}
