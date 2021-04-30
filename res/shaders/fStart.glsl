varying vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform vec4[3] LightPositionArray;
uniform float[3] LightBrightnessArray;

varying vec3 pos;
varying vec3 N;
varying vec3 orig;
varying vec3 down;

vec4 PointLight(float decay, float a, float b, float c) {
    vec3 Lvec = LightPositionArray[0].xyz - pos;
    float brightness = LightBrightnessArray[0];
    vec3 avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0) * vec3(1.0, 1.0, 1.0);

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize(Lvec);   // Direction to the light source
    vec3 E = normalize(-pos);   // Direction to the eye/camera
    vec3 H = normalize(L + E);  // Halfway vector

    float d = length(Lvec);

    float Kd = max(dot(L, N), 0.0);
    vec3 diffuse = Kd * DiffuseProduct;

    float Ks = pow(max(dot(N, H), 0.0), Shininess);
    vec3 specular = Ks * avera;

    if(dot(L, N) < 0.0) {
        specular = vec3(0.0, 0.0, 0.0);
    } 

    float decayconstant = decay / (a*d*d + b*d + c);
    return decayconstant * vec4(brightness * (diffuse + specular), 0.0);
}

vec4 DirectionalLight(float decay, float a, float b, float c) {
    vec3 Lvec = LightPositionArray[1].xyz - orig;
    float brightness = LightBrightnessArray[1];
    vec3 avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0) * vec3(1.0, 1.0, 1.0);

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize(Lvec);   // Direction to the light source
    vec3 E = normalize(-pos);   // Direction to the eye/camera
    vec3 H = normalize(L + E);  // Halfway vector

    float d = length(Lvec);

    float Kd = max(dot(L, N), 0.0);
    vec3 diffuse = Kd * DiffuseProduct;

    float Ks = pow(max(dot(N, H), 0.0), Shininess);
    vec3 specular = Ks * avera;

    if(dot(L, N) < 0.0) {
        specular = vec3(0.0, 0.0, 0.0);
    } 

    float decayconstant = decay / (a*d*d + b*d + c);
    return decayconstant * vec4(brightness  * (diffuse + specular), 0.0);
}

vec4 SpotLight(float decay, float a, float b, float c, float ang) {
    vec3 Lvec = LightPositionArray[2].xyz - pos;

    // if (acos(dot(normalize(-Lvec), down)) > ang) {
    //     return vec4(0.0,0.0,0.0,0.0);
    //     ;
    // }
    if (acos(dot(normalize(-Lvec), down))> ang) {
        return vec4(0.0,0.0,0.0,0.0);
        ;
    }
    
    
    float brightness = LightBrightnessArray[2];
    vec3 avera = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0) * vec3(1.0, 1.0, 1.0);

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize(Lvec);   // Direction to the light source
    vec3 E = normalize(-pos);   // Direction to the eye/camera
    vec3 H = normalize(L + E);  // Halfway vector

    float d = length(Lvec);

    float Kd = max(dot(L, N), 0.0);
    vec3 diffuse = Kd * DiffuseProduct;

    float Ks = pow(max(dot(N, H), 0.0), Shininess);
    vec3 specular = Ks * avera;

    if(dot(L, N) < 0.0) {
        specular = vec3(0.0, 0.0, 0.0);
    } 

    float decayconstant = decay / (a*d*d + b*d + c);
    return decayconstant * vec4(brightness * (diffuse + specular), 0.0);
    
}

void main() {
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec3 ambient = AmbientProduct;

    vec4 finalColor = 0.4*vec4(globalAmbient + ambient, 1.0);
    
    finalColor += PointLight(90.0, 1.0, 1.0, 1.0);
    finalColor += DirectionalLight(30.0, 1.0, 1.0, 1.0);
    finalColor += SpotLight(10.0, 0.02, 1.0, 1.0, 0.65);

    gl_FragColor = finalColor * texture2D(texture, texCoord * 2.0);

}
