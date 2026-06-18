#version 410 core

out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D u_Surface;
uniform vec2 u_UVOffset;
uniform vec2 u_UVScale;
uniform float u_Alpha;

void main()
{
    vec2 scaledUV = v_TexCoord * u_UVScale;
    vec2 scrolledUV = scaledUV + u_UVOffset;
    FragColor = texture(u_Surface, scrolledUV) * vec4(1.0, 1.0, 1.0, u_Alpha);
}
