#version 330 core

uniform sampler2D u_Tex;

uniform vec4 u_Color;

in vec2 v_TexCoord;

out vec4 o_Color;

void main() {
	vec4 color = texture( u_Tex, v_TexCoord );
	o_Color = vec4( u_Color.rgb, color.r * u_Color.a );
}
