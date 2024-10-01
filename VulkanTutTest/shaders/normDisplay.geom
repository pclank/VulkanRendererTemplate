#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 3) out;

layout(location = 0) in VS_OUT {
    mat4 geomProj;
    vec3 geomNorm;
} gs_in[];

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return -normalize(cross(a, b));
}

vec4 GetCentroid()
{
    return (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3;
}

const float MAGNITUDE = 0.1f;

void main()
{
    vec4 triCentroid = GetCentroid();

    gl_Position = gs_in[0].geomProj * triCentroid;
    EmitVertex();
    gl_Position = gs_in[0].geomProj * (triCentroid + vec4(GetNormal(), 0.0f) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();

    // gl_Position = gs_in[0].geomProj * gl_in[0].gl_Position;
    // EmitVertex();
    // gl_Position = gs_in[0].geomProj * (gl_in[0].gl_Position + vec4(gs_in[0].geomNorm, 0.0f) * MAGNITUDE);
    // EmitVertex();
    // EndPrimitive();
    // gl_Position = gs_in[0].geomProj * gl_in[1].gl_Position;
    // EmitVertex();
    // gl_Position = gs_in[0].geomProj * (gl_in[1].gl_Position + vec4(gs_in[1].geomNorm, 0.0f) * MAGNITUDE);
    // EmitVertex();
    // EndPrimitive();
    // gl_Position = gs_in[0].geomProj * gl_in[2].gl_Position;
    // EmitVertex();
    // gl_Position = gs_in[0].geomProj * (gl_in[2].gl_Position + vec4(gs_in[2].geomNorm, 0.0f) * MAGNITUDE);
    // EmitVertex();
    // EndPrimitive();
}