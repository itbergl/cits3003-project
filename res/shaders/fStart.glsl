varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform vec4[2] LightPositionArray;
uniform float[2] LightBrightnessArray;

varying vec3 pos;
varying vec3 N;
varying vec3 orig;

void main() {

    // The vector to the light from the vertex    
    vec3 lightPositions[2];
    lightPositions[0] = LightPositionArray[0].xyz - pos;
    lightPositions[1] = LightPositionArray[1].xyz - orig;

    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;
    vec4 color = vec4(globalAmbient + ambient, 1.0);
    // vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    float constant = 10.0;
    float avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0);

    for(int i = 0; i < 2; i++) {

        vec3 Lvec = lightPositions[i];

        // Unit direction vectors for Blinn-Phong shading calculation
        vec3 L = normalize(Lvec);   // Direction to the light source
        vec3 E = normalize(-pos);   // Direction to the eye/camera
        vec3 H = normalize(L + E);  // Halfway vector

        float distance = length(Lvec);

        float Kd = max(dot(L, N), 0.0);
        vec3 diffuse = Kd * DiffuseProduct;

        float Ks = pow(max(dot(N, H), 0.0), Shininess);
        vec3 specular = Ks * SpecularProduct;

        if(dot(L, N) < 0.0) {
            specular = vec3(0.0, 0.0, 0.0);
        } 


        float decay = constant/(distance * distance);
        //CHANGE
        if (i==1) {
            distance = 1.0;
        }
        color = color + decay*vec4(LightBrightnessArray[i] * (diffuse + specular), 0.0);
    }
    gl_FragColor = color * texture2D(texture, texCoord * 2.0);

}
