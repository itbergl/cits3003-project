attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec3 fN;
varying vec3 fV;
varying vec3[3] fL;
varying vec3 spotlight_direction;

uniform mat4 ModelView;
uniform mat4 view;
uniform mat4 Projection;
uniform float texScale;

uniform vec4[3] LightPosition;
uniform mat4 SpotlightDirectionMatrix;

void main() {
    // vectors for lighting calulation
    fN = (ModelView * vec4(vNormal, 0.0)).xyz;
    fV = -(ModelView * vec4(vPosition, 1.0)).xyz;

    // vector to light sources
    fL[0] = (view * LightPosition[0]).xyz + fV;     //point
    fL[1] = (view * LightPosition[1]).xyz;          //directional
    fL[2] = (view * LightPosition[2]).xyz + fV;     //spotlight

    // direction the spotlight is aiming is a rotation of the downward vector
    spotlight_direction = (view * SpotlightDirectionMatrix * vec4(0.0, -1.0, 0.0, 0.0)).xyz;
    
    texCoord = texScale * vTexCoord;

    gl_Position = Projection * ModelView * vec4(vPosition, 1.0);;

}