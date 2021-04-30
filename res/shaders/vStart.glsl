attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec3 pos;
varying vec3 N;
varying vec3 orig;
varying vec3 dir;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform float texScale;
uniform mat4 view;
uniform mat4 SpotlightDirectionMatrix;

void main()
{
    vec4 vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    pos = (ModelView * vpos).xyz;
   
    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );

    orig = (view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    // dir = (view * SpotlightDirectionMatrix * vec4(0.0, -1.0, 0.0, 0.0)).xyz;
    dir = (view*SpotlightDirectionMatrix*vec4(0.0, -1.0, 0.0, 0.0)).xyz;
    // up = (view* vec4(0.0, 1.0, 0.0, 0.0)).xyz;

    gl_Position = Projection * ModelView * vpos;
    texCoord = texScale*vTexCoord;
}