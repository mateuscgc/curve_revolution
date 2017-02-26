attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec4 aColor;
attribute vec2 aTexCoord;

uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;
uniform float uPoly;

varying vec4 vColor;
varying vec3 vNormal;
varying vec3 vPosW;
varying vec2 vTexCoord;

void main() {
	vTexCoord = aTexCoord;
	vPosW = (uModelMatrix * vec4(aPosition, 1.0)).xyz;
	vNormal = normalize(uNormalMatrix * aNormal);
    
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
	
	vColor = vec4(pow(aColor.r, uPoly), pow(aColor.g, uPoly), pow(aColor.b, uPoly), 1.0);
}