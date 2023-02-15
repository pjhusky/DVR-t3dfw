#version 330 core
//#extension GL_GOOGLE_include_directive : enable

//layout(binding = 0) 
uniform sampler2D u_Tex;

//layout(location = 0) 
in vec2 v_TexCoord;

//layout(location = 0) 
out vec4 o_Color;

void main() {
	vec4 color = texture( u_Tex, v_TexCoord );
	//if ( color.a < 0.001 ) { discard; }
	//o_Color = vec4( 0.0, 0.5, 0.2, 0.7 );
	o_Color = color;
}
