#version 330 core

in vec2 v_TexCoord;
out vec4 o_Color;

uniform samplerBuffer u_roundRectsTB;
uniform samplerBuffer u_arrowsTB;

uniform ivec2 u_num_RoundRects_Arrows;

uniform vec2 u_scaleRatio;

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

float sdOrientedBox( in vec2 p, in vec2 a, in vec2 b, float th )
{
    float l = length(b-a);
    vec2  d = (b-a)/l;
    vec2  q = p-(a+b)*0.5;
          q = mat2(d.x,-d.y,d.y,d.x)*q;
          q = abs(q)-vec2(l*0.5,th);
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);    
}

float sdOrientedBoxRounded( in vec2 p, in vec2 a, in vec2 b, float th, in float r ) {
    return sdOrientedBox( p, a, b, th ) - r;
}


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

    for ( int i = 0;  i < u_num_RoundRects_Arrows.x; i++ ) { // round rects
        vec4 positions = texelFetch( u_roundRectsTB, i * 2 + 0 );
        vec2 startPos = positions.xy;
        vec2 endPos = positions.zw;
        vec4 attribs = texelFetch( u_roundRectsTB, i * 2 + 1 );
        float thickness = attribs.x;
        float roundness = attribs.y;
        float currDist = sdOrientedBoxRounded( p, startPos, endPos, thickness, roundness );
        d = min( d, currDist );
    }

    for ( int i = 0;  i < u_num_RoundRects_Arrows.y; i++ ) { // arrows
        vec4 positions = texelFetch( u_arrowsTB, i * 2 + 0 );
        vec2 startPos = positions.xy;
        vec2 endPos = positions.zw;
        vec4 attribs = texelFetch( u_roundRectsTB, i * 2 + 1 );
        float shaftThickness = attribs.x;
        float headThickness = attribs.y;
        float currDist = sdArrow( p, startPos, endPos, shaftThickness, headThickness );
        d = min( d, currDist );
    }

    d = step ( 0.0025, d ); // the bigger than 0, the "smoother" the (combined) shapes
    if ( d > 0.0 ) {
        discard;
        return;
    }
    vec3 col = vec3( 1.0 - d );

	o_Color = vec4(col, 1.0 - d);

}

void main2() {
    // normalized pixel coordinates
    vec2 p = ( v_TexCoord * 2.0 - 1.0 ) * u_scaleRatio;
    
    //p *= 1.4;
    
    float iTime = 0.0;
    
    // c  : center of box
    // he : half-extents of the box, vec2(width*0.5,height*0.5)
    // u  : orientation vector, sin-cos pair of rotation angle
    // r  : rounding radius
    // s  : stroke width
    vec2 c = vec2(cos(iTime*0.5),sin(iTime*0.5))*0.75;
    vec2 he = vec2(0.25,0.75);
    vec2 u = vec2(cos(iTime),sin(iTime));
//    float r = 0.125;
    float s = 0.005;
    float r = 0.01;

	//float d = distance_box(p, c, he, u) - r;
    //float d = sdOrientedBox( p, c-he, c+he, s ) - r;
    //float d = sdOrientedBox( p, vec2( 0.1, 0.4), vec2( 0.3, 0.4), s ) - r;
    //float d = sdOrientedBoxRounded( p, vec2( 0.1, 0.4), vec2( 0.3, 0.6), s, r );
    //float d = sdOrientedBoxRounded( p, vec2( 0.1, 0.4), vec2( 0.3, 0.4), s, r );
    
    float arrowThickness = 0.005;
    //float d = sdArrow( p, vec2( 0.1, 0.4), vec2( 0.8, 0.4), arrowThickness, arrowThickness+0.01 );    
    float dArrow = sdArrow( p, vec2( 0.9, 0.8), vec2( 0.35, 0.25), arrowThickness, arrowThickness+0.01 );    
    
    float dBox = sdOrientedBoxRounded( p, vec2( 0.7, 0.8), vec2( 1.1, 0.8), s, r );
    float d = min( dArrow, dBox );
    //float d = abs( dArrow ) * abs( dBox );
    //float d = abs( dArrow ) + abs( dBox );
    //float d = dArrow * dBox;
    //float d = dArrow + dBox;

    float dBox2 = sdOrientedBoxRounded( p, vec2( 0.1, 0.2), vec2( 0.4, 0.2), s, r );
    d = min( d, dBox2 );

    // colorize distance
//    vec3 col = vec3(1.0) - sign(d)*vec3(0.1,0.4,0.7);
//	col *= 1.0 - exp(-6.0*abs(d));
//	col *= 0.8 + 0.2*cos(120.0*d);
//	col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,0.015,stroke(d, s)) );

    //d = clamp( d, 0.0, 1.0 );
    //d = step ( 0.0, d );
    d = step ( 0.0025, d ); // the bigger than 0, the "smoother" the (combined) shapes
    if ( d > 0.0 ) {
        discard;
        return;
    }
    vec3 col = vec3( 1.0 - d );

	o_Color = vec4(col, 1.0 - d);
}


////#extension GL_GOOGLE_include_directive : enable
//
////layout(binding = 0) 
//uniform sampler2D u_Tex;
//
////layout(location = 0) 
//in vec2 v_TexCoord;
//
////layout(location = 0) 
//out vec4 o_Color;
//
//float sdOrientedBox( in vec2 p, in vec2 a, in vec2 b, float th ) {
//    float l = length(b-a);
//    vec2  d = (b-a)/l;
//    vec2  q = (p-(a+b)*0.5);
//          q = mat2(d.x,-d.y,d.y,d.x)*q;
//          q = abs(q)-vec2(l,th)*0.5;
//    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);    
//}
//
//float sdOrientedBoxRounded( in vec2 p, in vec2 a, in vec2 b, float th, in float r ) {
//    return sdOrientedBox( p, a, b, th ) - r;
//}
//
//float sdSegment( in vec2 p, in vec2 a, in vec2 b ) {
//    vec2 pa = p-a, ba = b-a;
//    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
//    return length( pa - ba*h );
//}
//
//float sdSegmentRounded( in vec2 p, in vec2 a, in vec2 b, in float r ) {
//    return sdSegment( p, a, b ) - r;
//}
//
//void main() {
//	//vec4 color = texture( u_Tex, v_TexCoord );
//    //float dist = 1.0 - sdOrientedBoxRounded( v_TexCoord.xy, vec2( 0.1, 0.1 ), vec2( 0.4, 0.2 ), 0.1, 0.02 );
//    float dist = 1.0 - sdOrientedBox( v_TexCoord.xy, vec2( 0.1, 0.1 ), vec2( 0.4, 0.2 ), 0.1, 0.02 );
//    dist = clamp( dist, 0.0, 1.0 );
//	vec4 color = vec4( 1.0, 1.0, 1.0, dist );
//    o_Color = color;
//    //o_Color = vec4( v_TexCoord.xy, 0.0, 1.0 );
//    //o_Color = vec4( gl_FragCoord.xy / vec2(2400.0, 1800.0), 0.0, 1.0 );
//}
//