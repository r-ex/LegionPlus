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
uniform sampler2D normalTexture;

void main()
{
  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * vec3(1, 1, 1);   // Amb color
  
  // Diffuse
  vec3 norm = normalize(vertNormal);
  vec3 normMap = normalize((texture(normalTexture,vertUVLayer).rgb * -2) + 1);
  normMap.z = 1;
  normMap = normalize(normMap);


  vec3 lightDir = normalize(inverse(view)[3].xyz - vertFragPos);
  float diff = 1 - max(dot(norm * normMap, lightDir), 0.0);
  vec3 diffuse = diff * vec3(1, 1, 1);  // Light color
  
  // Result
  if (diffuseLoaded == 1) {
    color = (ambient + diffuse) * texture(diffuseTexture, vertUVLayer).rgb;
  } else {
    color = (ambient + diffuse) * 0.6;
  }
}