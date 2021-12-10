#version 430 core
layout(location = 0) out vec3 color;

in vec3 vertColorFrag;
in vec3 vertFragPos;
in vec3 vertNormal;
in vec2 vertUVLayer;

uniform mat4 view;
uniform mat4 projection;

uniform int diffuseLoaded;
uniform sampler2D diffuseTexture;

void main()
{
  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * vec3(1, 1, 1);   // Amb color
  
  // Diffuse
  vec3 norm = normalize(vertNormal);
  vec3 lightDir = normalize(inverse(view)[3].xyz - vertFragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * vec3(1, 1, 1);  // Light color
  
  // Result
  if (diffuseLoaded == 1) {
    color = (ambient + diffuse) * texture(diffuseTexture, vertUVLayer).rgb;
  } else {
    color = (ambient + diffuse) * vec3(0.603, 0.603, 0.603);
  }
}