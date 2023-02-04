
#if 0
vec3 rand01(inout uvec3 x){                   // pseudo-random number generator
    for (int i=3; i-->0;) x = ((x>>8U)^x.yzx)*1103515245U;
    return vec3(x)*(1.0/float(0xffffffffU));
}
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
