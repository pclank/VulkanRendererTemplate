#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

float vpw = 1.0f; // Width, in pixels
float vph = 1.0f; // Height, in pixels

vec2 offset = vec2(-0.023500000000000434f, 0.9794000000000017f); // e.g. [-0.023500000000000434 0.9794000000000017], currently the same as the x/y offset in the mvMatrix
vec2 pitch = vec2(50.0f);  // e.g. [50 50]

layout(location = 0) out vec4 outColor;

void main()
{
    //float lX = gl_FragCoord.x / vpw;
    //float lY = gl_FragCoord.y / vph;
    //
    //float scaleFactor = 10000.0f;
    //
    //float offX = (scaleFactor * offset[0]) + gl_FragCoord.x;
    //float offY = (scaleFactor * offset[1]) + (1.0 - gl_FragCoord.y);
    //
    //if (int(mod(offX, pitch[0])) == 0 || int(mod(offY, pitch[1])) == 0)
    //{
    //    outColor = vec4(0.0, 0.0, 0.0, 0.5);
    //}
    //else
    //    outColor = vec4(1.0, 1.0, 1.0, 1.0);

    if (fract(fragTexCoord.x / 0.001f) < 0.01f || fract(fragTexCoord.y / 0.001f) < 0.01f)
        outColor = vec4(1.0f);
    else
        outColor = vec4(0.0f);
}