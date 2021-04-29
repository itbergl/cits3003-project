attribute vec3 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

varying vec2 texCoord;
varying vec3 pos;
varying vec3 N;
varying vec3 orig;
varying vec3 down;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform float texScale;


void main()
{
    vec4 vpos = vec4(vPosition, 1.0);

    // Transform vertex position into eye coordinates
    pos = (ModelView * vpos).xyz;
   
    // Transform vertex normal into eye coordinates (assumes scaling
    // is uniform across dimensions)
    N = normalize( (ModelView*vec4(vNormal, 0.0)).xyz );

    orig = (ModelView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 downpoint = (ModelView * vec4(0.0, -1.0, 0.0, 1.0)).xyz;
    down = normalize(downpoint-orig);
    //down = normalize(

    gl_Position = Projection * ModelView * vpos;
    texCoord = texScale*vTexCoord;
}