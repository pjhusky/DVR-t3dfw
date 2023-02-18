#version 330 core
#extension GL_GOOGLE_include_directive : enable

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;
uniform sampler3D u_gradientTex;
uniform sampler3D u_densityLoResTex;
uniform sampler2D u_emptySpaceTex;
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

    float numSkips = 0.0;

    ivec3 hiResDim = textureSize( u_densityTex, 0 );
    vec3 fHiResDim = vec3( hiResDim );

    ivec3 loResDim = textureSize( u_densityLoResTex, 0 );
    vec3 fLoResDim = vec3( loResDim );
    vec3 fRecipLoResDim = 1.0 / fLoResDim;

//    int mainDir = -1;
//    vec3 abs_vol_step_ray_unit = abs( vol_step_ray_unit );
//    if ( abs_vol_step_ray_unit.x >= abs_vol_step_ray_unit.y ) { //1. x >= y
//        if (abs_vol_step_ray_unit.x >= abs_vol_step_ray_unit.z) { //2. x >= z
//            mainDir = 0; // x biggest
//        } else { //2. x < z 
//            mainDir = 2; // z biggest
//        }
//    } else { //1. x < y
//        if (abs_vol_step_ray_unit.y >= abs_vol_step_ray_unit.z) { //2. y >= z
//            mainDir = 1; // y biggest
//        } else { //2. y < z
//            mainDir = 2; // z biggest
//        }
//    }

    vec3 normPlaneX = ( vol_step_ray.x > 0.0 ) ? vec3( 1.0, 0.0, 0.0 ) : vec3( -1.0,  0.0,  0.0 );
    vec3 normPlaneY = ( vol_step_ray.y > 0.0 ) ? vec3( 0.0, 1.0, 0.0 ) : vec3(  0.0, -1.0,  0.0 );
    vec3 normPlaneZ = ( vol_step_ray.z > 0.0 ) ? vec3( 0.0, 0.0, 1.0 ) : vec3(  0.0,  0.0, -1.0 );

//    float normPlaneXdecider = normPlaneX.x * 0.5 + 0.5;
//    float normPlaneYdecider = normPlaneY.y * 0.5 + 0.5;
//    float normPlaneZdecider = normPlaneZ.z * 0.5 + 0.5;
    vec3 normPlaneDecider = vec3( normPlaneX.x, normPlaneY.y, normPlaneZ.z ) * vec3(0.5) + vec3(0.5);

//    normPlaneX.x *= fRecipLoResDim.x;
//    normPlaneY.y *= fRecipLoResDim.y;
//    normPlaneZ.z *= fRecipLoResDim.z;

    vec3 curr_sample_pos_noRand = curr_sample_pos + 0.5 * vol_step_ray; // ensure that even with random -0.5 offset we are inside the volume
    int iStep = 0;
    for ( float currStep = 0.0; currStep < lenInVolume; currStep += RAY_STEP_SIZE, curr_sample_pos_noRand += vol_step_ray ) {

        // center hiResTexCoord to the parent-loresBlock's loResTexCoord center
        vec3 hi2lo = (curr_sample_pos_noRand * fHiResDim) * RECIP_BRICK_BLOCK_DIM;
        vec3 currSampleLoResTexelSpace = ( floor( hi2lo ) + 0.5 );
        vec3 loResTexCoord = currSampleLoResTexelSpace * fRecipLoResDim;

        vec2 minMaxLoRes = texture( u_densityLoResTex, loResTexCoord ).rg;
        minMaxLoRes *= HOUNSFIELD_UNIT_SCALE;

        float emptySpaceTableEntry = texture( u_emptySpaceTex, minMaxLoRes ).r;

        if ( emptySpaceTableEntry > 0.0 ) {
            
            //vec3 currSampleLoResTexelSpace = loResTexCoord * (fLoResDim);
            vec3 fractDistTxlS = fract( hi2lo );
            //vec3 fractDistTxlS = fract( currSampleLoResTexelSpace );
            //vec3 floorPosTxlS = floor( currSampleLoResTexelSpace );
            
//            fractDistTxlS.x = mix( fractDistTxlS.x, 1.0 - fractDistTxlS.x, normPlaneXdecider );
//            fractDistTxlS.y = mix( fractDistTxlS.y, 1.0 - fractDistTxlS.y, normPlaneYdecider );
//            fractDistTxlS.z = mix( fractDistTxlS.z, 1.0 - fractDistTxlS.z, normPlaneZdecider );

            fractDistTxlS = mix( fractDistTxlS, 1.0 - fractDistTxlS, normPlaneDecider );
            
//            float dx = dot( normPlaneX * fRecipLoResDim, vol_step_ray ) * fractDistTxlS.x;
//            float dy = dot( normPlaneY * fRecipLoResDim, vol_step_ray ) * fractDistTxlS.y;
//            float dz = dot( normPlaneZ * fRecipLoResDim, vol_step_ray ) * fractDistTxlS.z;
            float dx = dot( normPlaneX, vol_step_ray_unit ) * fractDistTxlS.x;
            float dy = dot( normPlaneY, vol_step_ray_unit ) * fractDistTxlS.y;
            float dz = dot( normPlaneZ, vol_step_ray_unit ) * fractDistTxlS.z;

            float minDelta = min( dx, min( dy, dz ) )  * BRICK_BLOCK_DIM;
            
            currStep += RAY_STEP_SIZE * minDelta;

            //curr_sample_pos_noRand += vol_step_ray * minDelta;
//            curr_sample_pos_noRand += 0.5 * vol_step_ray; // push into the dir of the ray since up to -0.5 backward rand movement possible

            curr_sample_pos_noRand += vol_step_ray * ( minDelta + 0.5 ); // push into the dir of the ray since up to -0.5 backward rand movement possible

            numSkips++;
            continue;
        }

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
//        o_fragColor.rgb = vec3( numSkips * 0.05 );
        //o_fragColor.rgb = vec3( 1.0 - ( lenInVolume - numSkips ) / lenInVolume );
    #endif

    //o_fragColor.a = 1.0;
    o_fragColor.a = 0.25;
}

