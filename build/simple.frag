#version 330 core

in vec3 Normal;
in vec3 fragPos;
uniform vec3 objectColor;
uniform vec3 light1Color;
uniform vec3 light2Color;
uniform vec3 light1Pos;
uniform vec3 light2Pos;
uniform vec3 eyePos;

out vec4 outColor;

void main()
{
    //LIGHT 1
    float ambientStrength = 0.6f;
    vec3 ambient1 = ambientStrength * light1Color;

    vec3 norm = normalize(Normal);
    vec3 light1Direction = normalize(light1Pos - fragPos);

    float diffVal = max(dot(norm, light1Direction), 0.0);
    vec3 diffuse1 = diffVal * light1Color;

    float specStrength = 2.5f;

    vec3 eyeDirection = normalize(eyePos - fragPos);
    vec3 reflectDirection = reflect(-light1Direction, norm);

    float specVal = pow(max(dot(eyeDirection, reflectDirection), 0.0), 32);
    vec3 specular1 = specStrength * specVal * light1Color;


    //LIGHT 2
    float aStrength = 0.2f;
    vec3 ambient2 = aStrength * light2Color;

    vec3 light2Direction = normalize(light2Pos - fragPos);

    float diffVal2 = max(dot(norm, light2Direction), 0.0);
    vec3 diffuse2 = diffVal2 * light2Color;

    float sStrength = 0.6f;

    vec3 reflect2Direction = reflect(-light2Direction, norm);

    float specVal2 = pow(max(dot(eyeDirection, reflect2Direction), 0.0), 32);
    vec3 specular2 = sStrength * specVal2 * light2Color;

    vec3 result = (ambient1 + ambient2 + diffuse1 + diffuse2 + specular1 + specular2) * objectColor;
    outColor = vec4(result,1.0);

}
