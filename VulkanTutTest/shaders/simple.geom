#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

const int MAX_BONES = 120;                      // We need a maximum number, and 120 should be safe for the vast majority of rigs

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 inBoneTransforms[MAX_BONES];
    float time;
    bool explode;
} ubo;

layout(location = 1) in VS_OUT {
    vec2 geomTexCoord;
    vec3 geomNorm;
    vec3 geomPos;
} gs_in[];

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm;
layout(location = 3) out vec3 fragPos;

vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return -normalize(cross(a, b));
}

vec4 explode(vec4 position, vec3 normal)
{
    float time = ubo.time;
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0f) / 2.0f) * magnitude; 
    return position + vec4(direction, 0.0f);
}

void main()
{
    if (!ubo.explode)
    {
        gl_Position = gl_in[0].gl_Position;
        fragTexCoord = gs_in[0].geomTexCoord;
        fragNorm = gs_in[0].geomNorm;
        fragPos = gs_in[0].geomPos;
        EmitVertex();
        gl_Position = gl_in[1].gl_Position;
        fragTexCoord = gs_in[1].geomTexCoord;
        fragNorm = gs_in[1].geomNorm;
        fragPos = gs_in[1].geomPos;
        EmitVertex();
        gl_Position = gl_in[2].gl_Position;
        fragTexCoord = gs_in[2].geomTexCoord;
        fragNorm = gs_in[2].geomNorm;
        fragPos = gs_in[2].geomPos;
        EmitVertex();
        EndPrimitive();

        return;
    }

    vec3 normal = GetNormal();

    gl_Position = explode(gl_in[0].gl_Position, normal);
    fragTexCoord = gs_in[0].geomTexCoord;
    fragNorm = gs_in[0].geomNorm;
    fragPos = gs_in[0].geomPos;
    EmitVertex();
    gl_Position = explode(gl_in[1].gl_Position, normal);
    fragTexCoord = gs_in[1].geomTexCoord;
    fragNorm = gs_in[1].geomNorm;
    fragPos = gs_in[1].geomPos;
    EmitVertex();
    gl_Position = explode(gl_in[2].gl_Position, normal);
    fragTexCoord = gs_in[2].geomTexCoord;
    fragNorm = gs_in[2].geomNorm;
    fragPos = gs_in[2].geomPos;
    EmitVertex();
    EndPrimitive();
}