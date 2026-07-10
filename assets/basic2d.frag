#version 300 es

// use layout as described by rlgl.h
precision mediump float;
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uvOffset; // y offset, for spritesheets

void main()
{
    uvOffset;
    vec4 texelColor = texture(texture0, fragTexCoord + vec2(0.0, uvOffset));
    finalColor = texelColor*colDiffuse*fragColor;
}
