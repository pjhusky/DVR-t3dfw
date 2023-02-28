#version 330 core

in vec2 v_TexCoord;
out vec4 o_Color;

uniform samplerBuffer u_roundRectsTB;
uniform samplerBuffer u_arrowsTB;

uniform ivec2 u_num_RoundRects_Arrows;

uniform vec2 u_scaleRatio;

// https://www.shadertoy.com/view/cts3W2
vec2 skew(vec2 v)
{
	return vec2(-v.y, v.x);
}

float distance_aabb(vec2 p, vec2 he)
{
	vec2 d = abs(p) - he;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float distance_box(vec2 p, vec2 c, vec2 he, vec2 u)
{
    mat2 m = transpose(mat2(u, skew(u)));
    p = p - c;
    p = m * p;
    return distance_aabb(p, he);
}

float stroke(float d, float s)
{
    return abs(d) - s;
}

// https://www.shadertoy.com/view/slj3Dd
// The arrow goes from a to b. It's thickness is w1. The arrow
// head's thickness is w2.
float sdArrow( in vec2 p, vec2 a, vec2 b, float w1, float w2 )
{
    // constant setup
    const float k = 3.0;   // arrow head ratio
	vec2  ba = b - a;
    float l2 = dot(ba,ba);
    float l = sqrt(l2);

    // pixel setup
    p = p-a;
    p = mat2(ba.x,-ba.y,ba.y,ba.x)*p/l;
    p.y = abs(p.y);
    vec2 pz = p-vec2(l-w2*k,w2);

    // === distance (four segments) === 

    vec2 q = p;
    q.x -= clamp( q.x, 0.0, l-w2*k );
    q.y -= w1;
    float di = dot(q,q);
    //----
    q = pz;
    q.y -= clamp( q.y, w1-w2, 0.0 );
    di = min( di, dot(q,q) );
    //----
    if( p.x<w1 ) // conditional is optional
    {
    q = p;
    q.y -= clamp( q.y, 0.0, w1 );
    di = min( di, dot(q,q) );
    }
    //----
    if( pz.x>0.0 ) // conditional is optional
    {
    q = pz;
    q -= vec2(k,-1.0)*clamp( (q.x*k-q.y)/(k*k+1.0), 0.0, w2 );
    di = min( di, dot(q,q) );
    }
    
    // === sign === 
    
    float si = 1.0;
    float z = l - p.x;
    if( min(p.x,z)>0.0 ) //if( p.x>0.0 && z>0.0 )
    {
      float h = (pz.x<0.0) ? w1 : z/k;
      if( p.y<h ) si = -1.0;
    }
    return si*sqrt(di);
}

void main() {

    vec2 p = ( v_TexCoord * 2.0 - 1.0 ) * u_scaleRatio;
    float d = 1000000.0; // large number

    #if 1
    for ( int i = 0;  i < u_num_RoundRects_Arrows.x; i++ ) { // round rects
    //for ( int i = 0;  i < 4; i++ ) { // round rects
        vec4 positions = texelFetch( u_roundRectsTB, i * 2 + 0 );
        vec2 startPos = positions.xy;
        vec2 endPos = positions.zw;
        vec4 attribs = texelFetch( u_roundRectsTB, i * 2 + 1 );
        float thickness = attribs.x;
        float roundness = attribs.y;

//        if ( distance( startPos, endPos ) < thickness ) { continue; }

        float currDist = distance_box( p, 0.5 * ( startPos + endPos ), vec2( distance( startPos, endPos ) * 0.5, thickness ), vec2( 1.0, 0.0 ) ) - roundness;
        currDist = max( 0.0, currDist );
        d = min( d, currDist );
        
    }
    #endif

    for ( int i = 0;  i < u_num_RoundRects_Arrows.y; i++ ) { // arrows
        vec4 positions = texelFetch( u_arrowsTB, i * 2 + 0 );
        vec2 startPos = positions.xy;
        vec2 endPos = positions.zw;
        vec4 attribs = texelFetch( u_arrowsTB, i * 2 + 1 );
        float shaftThickness = attribs.x;
        float headThickness = attribs.y;

        if ( distance( startPos, endPos ) < shaftThickness ) { continue; }

        float currDist = sdArrow( p, startPos, endPos, shaftThickness, headThickness );
        d = min( d, currDist );
    }

    //#if 1
    //d = step ( 0.0025, d ); // the bigger than 0, the "smoother" the (combined) shapes
    //d = smoothstep ( 0.0025, 1.0, d );
    if ( d > 0.0 ) {
        discard;
        return;
    }
    //#else
    d = clamp( d * d, 0.0, 1.0 );
    //#endif
    vec3 col = vec3( 1.0 - d );

	o_Color = vec4(col, 1.0 - d);

}
