#version 330 core

in vec3 v_ndcPos;
in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;

// L top
// L btm
// R btm
// R top
uniform vec4 u_fpDist_OS[4];


uniform vec3 u_minMaxScaleVal;
uniform vec4 u_camPos_OS;
uniform vec3 u_volDimRatio;
uniform float u_recipTexDim;

// intersect ray with a box
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm

int intersectBox(vec3 S, vec3 v, vec3 boxmin, vec3 boxmax, out float tnear, out float tfar)
{
    // compute intersection of ray with all six bbox planes
    vec3 inv_dir = vec3(1.0) / v;
    vec3 tbot = inv_dir * (boxmin - S);
    vec3 ttop = inv_dir * (boxmax - S);

    // re-order intersections to find smallest and largest on each axis
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);

    // find the largest tmin and the smallest tmax
    float largest_tmin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallest_tmax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));

    tnear = largest_tmin;
    tfar = smallest_tmax;

	return int(smallest_tmax > largest_tmin);
}

void main() {

    vec4 fp_x_ipol_top = mix( u_fpDist_OS[ 0 ], u_fpDist_OS[ 3 ], v_coord3d.x );
    vec4 fp_x_ipol_btm = mix( u_fpDist_OS[ 1 ], u_fpDist_OS[ 2 ], v_coord3d.x );
    vec4 fp_xy_ipol = mix(fp_x_ipol_btm, fp_x_ipol_top, v_coord3d.y);

    // project interpolated homogeneous OS pos to real OS
    vec3 ray_far_OS = fp_xy_ipol.xyz / fp_xy_ipol.w;

    vec3 ray_end   = ray_far_OS;
    vec3 ray_start = u_camPos_OS.xyz;

    vec3 ray_dir = ray_end - ray_start;

    ray_dir = normalize( ray_dir ); // not strictly necessary but maybe better for tnear and tfar meaningfulness

    float tnear, tfar;
	//int hit = intersectBox(ray_start, ray_dir, vec3(-1.0), vec3(1.0), tnear, tfar);
    int hit = intersectBox(ray_start, ray_dir, -u_volDimRatio, u_volDimRatio, tnear, tfar);
    if ( hit == 0 ) { discard; return; }


    tnear = max( 0.0, tnear );
    tfar = max( tfar, tnear );
    vec3 curr_sample_pos = ray_start + tnear * ray_dir;
    vec3 end_sample_pos = ray_start + tfar * ray_dir;

    vec4 color = vec4( 0.0 );
    float numPosDensities = 1.0;
    
#if 0 // fixed number of steps (denser sampling on shorter distances, less dense on larger dists)

    curr_sample_pos = curr_sample_pos / u_volDimRatio * 0.5 + 0.5;
    end_sample_pos = end_sample_pos / u_volDimRatio * 0.5 + 0.5;

    float maxSteps = 300.0;
    vec3 ray_dir_step = ( end_sample_pos - curr_sample_pos ) / maxSteps;

    for ( float currStep = 0.0; currStep < maxSteps; currStep += 1.0 ) {
        curr_sample_pos += ray_dir_step;
        float texVal = texture( u_densityTex, curr_sample_pos ).r;
        
        // texVal = (texVal - u_minMaxScaleVal.x) * u_minMaxScaleVal.z; // we can do this once after the loop
        color += vec4( texVal );
        numPosDensities += (texVal > 0.0) ? 0.25 : 0.0;
    }

#else // fixed step size (equal sampling density everywhere) => generally faster than the above approach!!!

    // perform transformation into "non-square" dataset only once instead of at each sampling position
    curr_sample_pos = curr_sample_pos / u_volDimRatio * 0.5 + 0.5;
    end_sample_pos = end_sample_pos / u_volDimRatio * 0.5 + 0.5;

    float lenInVolume = length( end_sample_pos - curr_sample_pos );    
    vec3 vol_step_ray = normalize( end_sample_pos - curr_sample_pos );

//    for ( float currStep = 0.0; currStep < lenInVolume; currStep += 0.0033 ) {
//        vec3 ray_step_pos = curr_sample_pos + currStep * vol_step_ray;
//        float texVal = texture( u_densityTex, ray_step_pos ).r;
//        color += vec4( texVal );
//        numPosDensities += (texVal > 0.0) ? 0.25 : 0.0;
//    }

    vol_step_ray *= 0.0033; // max steps roughly 300
    for ( float currStep = 0.0; currStep < lenInVolume; currStep += 0.0033 ) {
        curr_sample_pos += vol_step_ray;
        float texVal = texture( u_densityTex, curr_sample_pos ).r;
        // texVal = (texVal - u_minMaxScaleVal.x) * u_minMaxScaleVal.z; // we can do this once after the loop
        color += vec4( texVal );
        numPosDensities += (texVal > 0.0) ? 0.25 : 0.0;
    }

#endif
    
    color = ( color - u_minMaxScaleVal.x ) * u_minMaxScaleVal.z; // map volume-tex values to 0-1 after the loop

    o_fragColor.rgb = color.rgb / numPosDensities;
    o_fragColor.a = 1.0;
}
