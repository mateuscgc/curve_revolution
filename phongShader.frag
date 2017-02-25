
uniform vec3 uLPos;
uniform vec3 uLColor;
//uniform vec3 uCameraPos;

varying vec4 vColor;
varying vec3 vNormal;
varying vec3 vPosW;

void main() {
	vec4 lColor = vec4(uLColor, 1.0);
    vec4 matAmb = vColor;
    vec4 matDif = vColor;
    vec4 matSpec = vColor;

    vec4 ambient = vec4(lColor.rgb * matAmb.rgb, matAmb.a);

    vec3 vL = normalize(uLPos - vPosW);
    float teta = max(dot(vL, vNormal), 0.0);

    vec4 diffuse = vec4(lColor.rgb * matDif.rgb * teta, matDif.a);

    vec3 vV = normalize(uLPos-vPosW);
    vec3 vR = normalize(reflect(-vL, vNormal));
    float omega = max(dot(vV, vR), 0.0);
    vec4 specular = vec4(lColor.rgb * matSpec.rgb * pow(omega,20.0), matSpec.a);

    //gl_FragColor = clamp(ambient + diffuse + specular, 0.0, 1.0);
    gl_FragColor = clamp(diffuse + specular, 0.0, 1.0);
}
