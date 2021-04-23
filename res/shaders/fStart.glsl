
varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform vec4 LightPosition1;
uniform vec4 LightPosition2;

varying vec3 pos;
varying vec3 N;
varying vec4 camerapos;

void main()
{

    // The vector to the light from the vertex    
    vec3 LvecArray[2];
    LvecArray[0] = LightPosition1.xyz - pos;
    LvecArray[1] = (LightPosition2.xyz-pos);
    
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;
    vec4 color = vec4(globalAmbient  + ambient, 1.0);
    float constant = 10.0;
    float avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b)/3.0);

    for (int i=0; i < 2; i++) {
        vec3 Lvec = LvecArray[i];
        // Unit direction vectors for Blinn-Phong shading calculation
        vec3 L = normalize( Lvec );   // Direction to the light source
        vec3 E = normalize( -pos );   // Direction to the eye/camera
        vec3 H = normalize( L + E );  // Halfway vector
        
        float distance = length(Lvec);


        float Kd = max( dot(L, N), 0.0);
        vec3 diffuse = Kd*DiffuseProduct;

        float Ks = pow( max(dot(N, H), 0.0), Shininess );
        
        vec3  specular = Ks * avera*vec3(1.0,1.0,1.0);

        
        if (dot(L, N) < 0.0 ) {
            specular = vec3(0.0, 0.0, 0.0);
        } 

       
        // globalAmbient is independent of distance from the light source

        color = color + vec4((constant)*(1.0/(distance*distance)*(diffuse + specular)),1.0);
    }

    gl_FragColor = color * texture2D( texture, texCoord * 2.0 );
    
}
