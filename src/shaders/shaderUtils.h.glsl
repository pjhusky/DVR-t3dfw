
#if 0
    vec3 rand01(inout uvec3 x) {                   // pseudo-random number generator
        for (int i=3; i-->0;) x = ((x>>8U)^x.yzx)*1103515245U;
        return vec3(x)*(1.0/float(0xffffffffU));
    }
#elif 0 // DEBUG
    vec3 rand01(inout uvec3 x) { return vec3( 0.5 ); }
#else
    // http://www.jcgt.org/published/0009/03/02/
    // https://www.shadertoy.com/view/XlGcRh
    vec3 rand01( inout uvec3 v ) {
        v = v * 1664525u + 1013904223u;

        v.x += v.y*v.z;
        v.y += v.z*v.x;
        v.z += v.x*v.y;

        v ^= ( v >> 16u );

        v.x += v.y*v.z;
        v.y += v.z*v.x;
        v.z += v.x*v.y;

        vec3 fval = vec3( v.x, v.y, v.z );
        return fval * ( 1.0f / 0xffffffffU );
    }
#endif

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
