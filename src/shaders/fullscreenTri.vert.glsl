#version 330 core
//#extension GL_GOOGLE_include_directive : enable

//layout(location = 0) 
out vec2 v_TexCoord;

out gl_PerVertex{
    vec4 gl_Position;
};

void main() {
    // Draw a triangle that spans across the whole render target
    // Winding order is irrelevant as we set CULL_MODE_NONE
    // Note that we can calculate UV coords from NDC by UV = (NDC + 1) / 2 (see BRDF.{frag, psh})

// #if ( RASTER_MODE == RASTER_MODE_QUAD )
// 	vec2 Pos[6];
//     Pos[0] = vec2(-1.0, -1.0);
//     Pos[1] = vec2(-1.0, +1.0);
//     Pos[2] = vec2(+1.0, -1.0);
//     Pos[3] = vec2(-1.0, +1.0);
//     Pos[4] = vec2(+1.0, -1.0);
//     Pos[5] = vec2(+1.0, +1.0);
// 
//     vec2 UV[6];
//     UV[0] = vec2(+0.0, +1.0);
//     UV[1] = vec2(+0.0, +0.0);
//     UV[2] = vec2(+1.0, +1.0);
//     UV[3] = vec2(+0.0, +0.0);
//     UV[4] = vec2(+1.0, +1.0);
//     UV[5] = vec2(+1.0, +0.0);
// 
// 	out_NDC = UV[gl_VertexID];
// 	gl_Position = vec4(Pos[gl_VertexID], 0, 1);
// #elif (RASTER_MODE == RASTER_MODE_TRI_ARRAY)
// 
// 	vec2 Pos[3];
//     Pos[0] = vec2(-1.0, +1.0);
//     Pos[1] = vec2(-1.0, -3.0);
//     Pos[2] = vec2(+3.0, +1.0);
// 
//     vec2 UV[3];
//     UV[0] = vec2(+0.0, +0.0);
//     UV[1] = vec2(+0.0, +2.0);
//     UV[2] = vec2(+2.0, +0.0);
// 
// 	out_NDC = UV[gl_VertexID];
// 	gl_Position = vec4(Pos[gl_VertexID], 0, 1);
// #elif (RASTER_MODE == RASTER_MODE_TRI_CALC)

    int x =  4 * (gl_VertexID / 2) - 1;
    int y = -4 * (gl_VertexID % 2) + 1;
    //v_TexCoord = vec2( gl_VertexID / 2, 1 - gl_VertexID % 2 ) * 2.0f;
    v_TexCoord = vec2( x * 0.5 + 0.5, y * 0.5 + 0.5 );
    gl_Position = vec4( x, y, -0.999999, 1.0 );

// #endif

}
