
varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;

varying vec3 pos;
varying vec3 N;

void main()
{

    // The vector to the light from the vertex    
    vec3 Lvec = LightPosition1.xyz - pos;
    vec3 Lvec2 = LightPosition2.xyz;
    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize( Lvec );   // Direction to the light source
    vec3 E = normalize( -pos );   // Direction to the eye/camera
    vec3 H = normalize( L + E );  // Halfway vector

    vec3 L2 = normalize( Lvec2 );   // Direction to the light source
    vec3 E2 = normalize( -pos );   // Direction to the eye/camera
    vec3 H2 = normalize( L2 + E2 );  // Halfway vector


    float distance = length(Lvec);
    float distance2 = length(Lvec2);
	float constant = 10.0;


    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;

    float Kd = max( dot(L, N), 0.0);
    vec3  diffuse = Kd*DiffuseProduct;

    float Ks = pow( max(dot(N, H), 0.0), Shininess );
    float avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b)/3.0);
    vec3  specular = Ks * avera*vec3(1.0,1.0,1.0);

    float Kd2 = max( dot(L2,N), 0.0);
    vec3  diffuse2 = Kd2*DiffuseProduct;

    float Ks2 = pow( max(dot(N,H2),0.0), Shininess );
    float avera2 = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b)/3.0);
    vec3  specular2 = Ks2 * avera2*vec3(1.0,1.0,1.0);
    
    
    if (dot(L, N) < 0.0 ) {
	    specular = vec3(0.0, 0.0, 0.0);
    } 

    // globalAmbient is independent of distance from the light source
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec4 color = vec4(globalAmbient  + ambient + (constant)*(
                                                        (1.0/(distance*distance)*(diffuse + specular))
                                                        +(1.0/(distance2*distance2)*(diffuse2 + specular2))
                                                        ),1.0);
    

    gl_FragColor = color * texture2D( texture, texCoord * 2.0 );
    
}
