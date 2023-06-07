#version 330

uniform sampler2D textureMap; 

in vec2 iTexCoord;
in vec4 l1;
in vec4 l2;
in vec4 v; 
in vec4 n;
out vec4 pixelColor; 

void main(void) {
    vec4 texColor=texture(textureMap,iTexCoord);

    vec4 n_l1 = normalize(l1);
    vec4 n_l2 = normalize(l2);
    vec4 n_v = normalize(v);
    vec4 n_n = normalize(n);

    vec4 n_r1 = reflect(n_l1, n_n);
    vec4 n_r2 = reflect(n_l2, n_n);

    float nl1 = max(0, dot(n_n,n_l1));
    float nl2 = max(0, dot(n_n,n_l2));
    float rv1 = pow(max(0, dot(n_r1,n_v)), 50);
    float rv2 = pow(max(0, dot(n_r2,n_v)), 50);

    vec4 kd = vec4(0.7, 0.85, 1, 1);
	vec4 ks = vec4(0.7, 0.85, 1, 1);

    vec4 La = texColor * vec4(0.5,0.7,1,1);
    vec4 Ld = texColor * vec4(1,1,1,1);
    vec4 Ls = texColor * vec4(1,1,1,1);

    pixelColor = La + Ld*vec4(kd.rgb * nl1, kd.a) + Ld*vec4(kd.rgb * nl2, kd.a) + Ls*vec4(ks.rgb * rv1, 0) + Ls*vec4(ks.rgb * rv2, 0);
}
