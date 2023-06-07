#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 light1;
uniform vec4 light2;

in vec4 vertex;
in vec4 normal;
in vec2 texCoord;
out vec4 l1;
out vec4 l2;
out vec4 v; 
out vec4 n;
out vec2 iTexCoord;

void main(void) {
    iTexCoord = texCoord;

    l1 = normalize(V*light1 - V*M*vertex); 
    l2 = normalize(V*light2 - V*M*vertex);
    v = normalize(V*M*vec4(0, 0, 0, 1) - V*M*vertex);
    n = normalize(V*M*normal);

    gl_Position=P*V*M*vertex;
}
