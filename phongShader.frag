uniform vec3 uLPos;
uniform vec3 uLColor;
uniform sampler2D tex;
uniform int uFill;

varying vec4 vColor;
varying vec3 vNormal;
varying vec3 vPosW;
varying vec2 vTexCoord;

void main() {
    vec4 color;
    if(uFill) color = vColor;
    else color = texture(tex, vTexCoord);
    vec4 lColor = vec4(uLColor, 1.0);
    vec4 matAmb = color;
    vec4 matDif = color;
    vec4 matSpec = color;

    vec4 ambient = vec4(pow(lColor.r * matAmb.r, 1.2), pow(lColor.g * matAmb.g, 1.2), pow(lColor.b * matAmb.b, 1.2), matAmb.a);

    vec3 vL = normalize(uLPos - vPosW);
    float teta = max(dot(vL, vNormal), 0.0);

    vec4 diffuse = vec4(lColor.rgb * matDif.rgb * teta, matDif.a);

    vec3 vV = normalize(uLPos-vPosW);
    vec3 vR = normalize(reflect(-vL, vNormal));
    float omega = max(dot(vV, vR), 0.0);
    vec4 specular = vec4(lColor.rgb * matSpec.rgb * pow(omega,20.0), matSpec.a);

    gl_FragColor = clamp(ambient + diffuse + specular, 0.0, 1.0);
}
