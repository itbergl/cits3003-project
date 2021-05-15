attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec3 fN;
varying vec3 fV;
varying vec3[3] fL;
varying vec3 spotlight_direction;

uniform mat4 model;
uniform mat4 view;
uniform mat4 Projection;
uniform float texScale;

uniform vec4[3] LightPosition;
uniform mat4 SpotlightDirectionMatrix;

void main() {
    vec4 vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    // pos = (ModelView * vpos).xyz;

    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    mat4 ModelView = view * model;
    fN = (ModelView * vec4(vNormal, 0.0)).xyz;
    fV = -(ModelView * vec4(vPosition, 1.0)).xyz;

    fL[0] = (view * LightPosition[0]).xyz + fV;
    fL[1] = (view * LightPosition[1]).xyz;
    fL[2] = (view * LightPosition[2]).xyz + fV;

    spotlight_direction = (view * SpotlightDirectionMatrix * vec4(0.0, -1.0, 0.0, 0.0)).xyz;

    // orig = (view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    // dir = (view * SpotlightDirectionMatrix * vec4(0.0, -1.0, 0.0, 0.0)).xyz;

    texCoord = texScale * vTexCoord;

    gl_Position = Projection * ModelView * vpos;

}