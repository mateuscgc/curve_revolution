attribute vec3 aPosition;
attribute vec4 aColor;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

varying vec4 vColor;
varying vec3 vNormal;
varying vec3 vPosW;

void main() {
	vPosW = (uModelMatrix * vec4(aPosition, 1.0)).xyz;
	vNormal = normalize(gl_NormalMatrix * gl_Normal);
    
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
	
	vColor = aColor;
}
