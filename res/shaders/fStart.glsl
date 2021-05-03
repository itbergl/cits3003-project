varying vec2 texCoord;

uniform sampler2D texture;
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform vec4[3] LightPositionArray;
uniform float[3] LightBrightnessArray;
uniform vec3[3] LightRGBrray;
uniform float SpotlightAngle;

varying vec3 pos;
varying vec3 N;
varying vec3 orig;
varying vec3 dir;

// when given Lvec - direction of incoming light, the lightnumber in the corresponding uniform variables,
// and constants describing the light reduction (decay/(a*d^2+b*d+c)), returns the amount of light added
// to a fragment as an rgba vec4

vec4 getLightContribution(vec3 Lvec, int lightNo, float decay, float a, float b, float c) {
    // average rgb value of specuare product
    vec3 average = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0) * vec3(1.0, 1.0, 1.0);
    //properties of the light
    vec3 rgb = LightRGBrray[lightNo];
    float brightness = LightBrightnessArray[lightNo];

    // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize(Lvec);   // Direction to the light source
    vec3 E = normalize(-pos);   // Direction to the eye/camera
    vec3 H = normalize(L + E);  // Halfway vector

    float d = length(Lvec);

    float Kd = max(dot(L, N), 0.0);
    vec3 diffuse = Kd * DiffuseProduct;

    float Ks = pow(max(dot(N, H), 0.0), Shininess);
    vec3 specular = Ks * average;

    if(dot(L, N) < 0.0) {
        specular = vec3(0.0, 0.0, 0.0);
    }

    //constant for light attenuation
    float decayconstant = decay / (a * d * d + b * d + c);
    return decayconstant * vec4(brightness * rgb * (diffuse + specular), 0.0);
}

void main() {
    //ambient light
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec3 ambient = AmbientProduct;
    vec4 color = 0.5* vec4(globalAmbient + ambient, 1.0);

    vec3 Lvec;
    //point light
    Lvec = LightPositionArray[0].xyz - pos;
    color += getLightContribution(Lvec, 0, 
                        90.0, 1.0, 1.0, 1.0);

    //directional light
    Lvec = LightPositionArray[1].xyz - orig;
    color += getLightContribution(Lvec, 1, 
                        30.0, 1.0, 1.0, 1.0);

    //spot light
    Lvec = LightPositionArray[2].xyz - pos;
    if(acos(dot(normalize(-Lvec), dir)) < SpotlightAngle) {
        if (LightBrightnessArray[2] > 0.00001) {
            color += getLightContribution(Lvec, 2, 
                                15.0, 0.02, 1.0, 1.0);
        }
    }

    gl_FragColor = color * texture2D(texture, texCoord * 2.0);
}
