#version 330 core

in vec3 position;
in vec3 normalPos;
uniform mat4 projectionMatrix;
uniform mat4 viewingMatrix;
uniform mat4 modelMatrix;

out vec3 fragPos;
out vec3 Normal;



void main()
{
    vec4 transformedPosition = projectionMatrix*viewingMatrix*modelMatrix*vec4(position,1.0f);
    gl_Position = transformedPosition;

    fragPos = vec3(modelMatrix * vec4(position, 1.0f));

    Normal = mat3(transpose(inverse(modelMatrix))) * normalPos;

}
