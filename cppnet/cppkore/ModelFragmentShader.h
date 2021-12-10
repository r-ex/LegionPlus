#pragma once

constexpr const char* ModelFragmentShader_Src =
"#version 430 core\n"
"layout(location=0)out vec3 color;"
"in vec3 vertColorFrag,vertFragPos,vertNormal;"
"in vec2 vertUVLayer;"
"uniform mat4 view,projection;"
"uniform int diffuseLoaded;"
"uniform sampler2D diffuseTexture;"
"void main()"
"{"
"float v=.1;"
"vec3 d=v*vec3(1,1,1),r=normalize(vertNormal),l=normalize(inverse(view)[3].rgb-vertFragPos);"
"float c=max(dot(r,l),0.);"
"vec3 m=c*vec3(1,1,1);"
"if(diffuseLoaded==1)"
"color=(d+m)*texture(diffuseTexture,vertUVLayer).rgb;"
"else"
" color=(d+m)*vec3(.603,.603,.603);"
"}";