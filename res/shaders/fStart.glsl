varying vec3 fN;
varying vec3[3] fL;
varying vec3 fV;
varying vec2 texCoord;
varying vec3 spotlight_direction;

uniform sampler2D texture;
uniform float SpotlightAngle;

uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float[3] LightBrightness;
uniform vec3[3] LightIntensity;
uniform float Shininess;

// when given Lvec - direction of incoming light, the lightnumber varying the corresponding uniform variables,
// and constants describing the light reduction (decay/(a*d^2+b*d+c)), returns the amount of light added
// to a fragment as an rgba vec4

vec3 getLightContribution(int lightNo, float decay, float a, float b, float c) {
    // average rgb value of specuare product
    vec3 SpecularProductAverage = ((SpecularProduct.r + SpecularProduct.g + SpecularProduct.b) / 3.0) * vec3(1.0, 1.0, 1.0);
        //properties of the light
    vec3 intensity = LightIntensity[lightNo];
    float brightness = LightBrightness[lightNo];

        // Unit direction vectors for Blinn-Phong shading calculation
    vec3 L = normalize(fL[lightNo]);   // Direction to the light source
    vec3 E = normalize(fV);   // Direction to the eye/camera
    vec3 H = normalize(L + E);  // Halfway vector
    vec3 N = normalize(fN);

    float d = length(fL[lightNo]);

    float Kd = max(dot(L, N), 0.0);
    vec3 diffuse = Kd * DiffuseProduct;

    float Ks = pow(max(dot(N, H), 0.0), Shininess);
    vec3 specular = Ks * SpecularProductAverage;

    if(dot(L, N) < 0.0) {
        specular = vec3(0.0, 0.0, 0.0);
    }
    //constant for light attenuation
    float decayconstant = decay / (a * d * d + b * d + c);
    // float decayconstant = 20.0 / d;

    return decayconstant * brightness * intensity * (diffuse + specular);
}

void main() {
    //ambient light
    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec3 ambient = AmbientProduct;
    vec3 output_colour = 0.5 * (globalAmbient + ambient);

    //point light
    output_colour += getLightContribution(0, 30.0, 1.0, 1.0, 1.0);

    //directional light
    output_colour += getLightContribution(1, 30.0, 1.0, 1.0, 1.0);

    //spotlight
    if(acos(dot(normalize(-fL[2]), spotlight_direction)) < SpotlightAngle) {
        output_colour += getLightContribution(2, 15.0, 0.02, 1.0, 1.0);
    }

    gl_FragColor = vec4(output_colour, 1.0) * texture2D(texture, texCoord * 2.0);
}
