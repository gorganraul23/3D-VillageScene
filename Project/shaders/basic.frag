#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
//in vec4 fPosEye;
//in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform vec3 pointLightPosition1;
uniform vec3 pointLightColor1;
uniform vec3 pointLightPosition2;
uniform vec3 pointLightColor2;
//uniform sampler2D shadowMap;
//fog
uniform float fogDensity;
uniform float is_light;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

float constant = 1.0f;
float linear = 0.09f;
float quadratic = 0.04f;

vec4 fPosEye;

void computeDirLight()
{
    //compute eye space coordinates
    fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));
    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);
    //compute ambient light
    ambient = ambientStrength * lightColor;
    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;

}

vec3 computePointLight1()
{
	vec3 lightDir = normalize(pointLightPosition1 - fPosition);

    //////////////
    fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(- fPosEye.xyz);
    /////////////

    float diff = max(dot(normalEye, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
   
    float distance    = length(pointLightPosition1 - fPosition);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    
 
    vec3 ambient2  = vec3(texture(diffuseTexture, fTexCoords));
    vec3 diffuse2  = diff * vec3(texture(diffuseTexture, fTexCoords));
    vec3 specular2 = spec * vec3(texture(specularTexture, fTexCoords));
    ambient2  *= attenuation;
    diffuse2  *= attenuation;
    specular2 *= attenuation;
    return (ambient2 + diffuse2 + specular2);
}
vec3 computePointLight2()
{
	vec3 lightDir = normalize(pointLightPosition2 - fPosition);

    //////////////
    fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(- fPosEye.xyz);
    /////////////

    float diff = max(dot(normalEye, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
   
    float distance    = length(pointLightPosition2 - fPosition);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    
 
    vec3 ambient2  = vec3(texture(diffuseTexture, fTexCoords));
    vec3 diffuse2  = diff * vec3(texture(diffuseTexture, fTexCoords));
    vec3 specular2 = spec * vec3(texture(specularTexture, fTexCoords));
    ambient2  *= attenuation;
    diffuse2  *= attenuation;
    specular2 *= attenuation;
    return (ambient2 + diffuse2 + specular2);
}

float computeFog()
{
    //float fogDensity = 0.01f;
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

    return clamp(fogFactor, 0.0f, 1.0f);
}

/*float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
        
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    
    // Check whether current frag pos is in shadow
    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    if (normalizedCoords.z > 1.0f)
        return 0.0f;

    return shadow;	
}*/

void main() 
{
    computeDirLight();

    //float shadow = computeShadow();

    ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	
    vec3 color = min((ambient + diffuse) + specular, 1.0f);

    if(is_light == 1.0f){
        color += computePointLight1();
        color += computePointLight2();
    }

    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    fColor = mix(fogColor, vec4(color, 1.0f), fogFactor);
   
}
