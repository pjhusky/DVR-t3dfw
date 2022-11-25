#version 330 core

in vec3 v_coord3d;
layout ( location = 0 ) out vec4 o_fragColor;

uniform sampler3D u_densityTex;

uniform vec3 u_minMaxScaleVal;
uniform vec4 u_camPos_OS;
uniform vec3 u_volDimRatio;

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
        // texVal = (texVal - u_minMaxScaleVal.x) * u_minMaxScaleVal.z; // we can do this once after the loop
        color += vec4( texVal );
        numPosDensities += (texVal > 0.0) ? 0.25 : 0.0;
    }
    
    color = ( color - u_minMaxScaleVal.x ) * u_minMaxScaleVal.z; // map volume-tex values to 0-1 after the loop

    o_fragColor = color / numPosDensities;
    o_fragColor.a = 1.0;
}
