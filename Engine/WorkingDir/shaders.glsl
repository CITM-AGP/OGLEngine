///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;		// in worldspace
out vec3 vNormal;		// in worldspace
out vec3 vViewDir;		// in worldspace
out vec3 vTangent;		// in worldspace
out vec3 vBitangent;	// in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vViewDir = vec3(uCameraPosition - vPosition);
	
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
    vTangent = normalize(vec3(uWorldMatrix * vec4(aTangent, 0.0)));
    vBitangent = normalize(vec3(uWorldMatrix * vec4(aBitangent, 0.0)));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition;		// in worldspace
in vec3 vNormal;		// in worldspace
in vec3 vViewDir;		// in worldspace
in vec3 vTangent;		// in worldspace
in vec3 vBitangent;		// in worldspace

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uBumpTexture;
uniform float uBumpiness;
uniform int noTexture;
uniform int noNormal;
uniform int noBump;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oDepth;
layout(location = 4) out vec4 oPosition;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

vec2 ReliefMapping(vec2 texCoords, mat3 TBN)
{
	int numSteps = 30;

	// Compute the view ray in texture space
	vec3 rayTexSpace = transpose(TBN) * normalize(vViewDir.xyz);

	// Increment
	float texSize = 256;
	vec3 rayIncrementTexSpace;
	rayIncrementTexSpace.xy = -1 * uBumpiness * rayTexSpace.xy / abs(rayTexSpace.z * texSize);
	rayIncrementTexSpace.z = 1.0 / numSteps;

	// Sampling state
	vec3 samplePositionTexspace = vec3(texCoords, 0.0);
	float sampledDepth = texture(uBumpTexture, samplePositionTexspace.xy).r;

	// Linear search
	for (int i = 0; i < numSteps && samplePositionTexspace.z < sampledDepth; ++i)
	{
		samplePositionTexspace += rayIncrementTexSpace;
		sampledDepth = texture(uBumpTexture, samplePositionTexspace.xy).r;
	}

	return samplePositionTexspace.xy;
}

void main()
{
	// Mat parameters
    vec3 specular = vec3(1.0); // color reflected by mat
    float shininess = 120.0; // how strong specular reflections are (more shininess harder and smaller spec)
	
	vec3 V = normalize(vViewDir.xyz);	// direction from pixel to camera
	vec3 T = normalize(vTangent);		// tangent
	vec3 B = normalize(vBitangent);		// bitangent
    vec3 N = normalize(vNormal);		// normal
	
	// Convert normal from tangent space to world space
	mat3 TBN = mat3(T, B, N);
	vec2 texCoords = vTexCoord;

	// Relief map
	if (noBump == 0.0)
		texCoords = ReliefMapping(vTexCoord, TBN);

	vec3 albedo = texture(uTexture, texCoords).rgb;	

	if (noTexture == 1)
		oAlbedo = vec4(0.5);

	// Normal map
	if (noNormal == 0.0)
	{
		vec3 tangentSpaceNormal = texture(uNormalMap, texCoords).xyz * 2.0 - vec3(1.0);
		N = TBN * tangentSpaceNormal;
	}
	
	// Ambient
    float ambientIntensity = 0.4;
    vec3 ambientColor = albedo.xyz * ambientIntensity;	
	vec3 finalDiffuse;
	vec3 finalSpecular;

	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 1.0f;
		vec3 L;
		vec3 diffuseColor;
		vec3 specularColor;
		// --- If we have a point light, attenuate according to distance ---
		if(uLight[i].type == 1)
		{
			float dist = length(uLight[i].position - vPosition);
			float linear = 0.05;
			float quadratic = 0.01;
			attenuation = 1.0 / (1.0 + linear * dist + quadratic * dist * dist);
			L = normalize(uLight[i].position - vPosition); // Light direction 
		}
	    else    
			L = normalize(uLight[i].direction); // Light direction 

	    vec3 R = reflect(-L, N); // reflected vector
	    
	    // Diffuse
	    float diffuseIntensity = max(dot(N, L), 0.0);
	    diffuseColor += albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	    // Specular
		float specularIntensity = pow(max(dot(V, R), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;

		diffuseColor*=attenuation;
		specularColor*=attenuation;

		finalDiffuse += diffuseColor;
		finalSpecular += specularColor;
	}

	finalDiffuse /= uLightCount;
	finalSpecular /= uLightCount;

	// Final outputs
    oColor = vec4(ambientColor + finalDiffuse + finalSpecular, 1.0);

    oNormals = vec4(normalize(vNormal), 1.0); 
	oAlbedo = texture(uTexture, vTexCoord);

	float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
	oDepth = vec4(vec3(depth), 1.0);

	oPosition = vec4(vec3(vPosition), 1.0);
}

#endif
#endif

// ------------------------------------------------------------------------------------
// ------------------------- DEFERRED SHADING -----------------------------------------
// ------------------------------------------------------------------------------------

#ifdef GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;
layout(location=3) in vec3 aTangent;
layout(location=4) in vec3 aBitangent;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;		// in worldspace
out vec3 vNormal;		// in worldspace
out vec3 vViewDir;		// in worldspace
out vec3 vTangent;		// in worldspace
out vec3 vBitangent;	// in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vViewDir = vec3(uCameraPosition - vPosition);
	
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
    vTangent = normalize(vec3(uWorldMatrix * vec4(aTangent, 0.0)));
    vBitangent = normalize(vec3(uWorldMatrix * vec4(aBitangent, 0.0)));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition;		// in worldspace
in vec3 vNormal;		// in worldspace
in vec3 vViewDir;		// in worldspace
in vec3 vTangent;		// in worldspace
in vec3 vBitangent;		// in worldspace

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uBumpTexture;
uniform float uBumpiness;
uniform int noTexture;
uniform int noNormal;
uniform int noBump;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oDepth;
layout(location = 4) out vec4 oPosition;

float near = 0.1; 
float far  = 100.0; 
  
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

vec2 ReliefMapping(vec2 texCoords, mat3 TBN)
{
	int numSteps = 30;

	// Compute the view ray in texture space
	vec3 rayTexSpace = transpose(TBN) * normalize(vViewDir.xyz);

	// Increment
	float texSize = 256;
	vec3 rayIncrementTexSpace;
	rayIncrementTexSpace.xy = -1 * uBumpiness * rayTexSpace.xy / abs(rayTexSpace.z * texSize);
	rayIncrementTexSpace.z = 1.0 / numSteps;

	// Sampling state
	vec3 samplePositionTexspace = vec3(texCoords, 0.0);
	float sampledDepth = texture(uBumpTexture, samplePositionTexspace.xy).r;

	// Linear search
	for (int i = 0; i < numSteps && samplePositionTexspace.z < sampledDepth; ++i)
	{
		samplePositionTexspace += rayIncrementTexSpace;
		sampledDepth = texture(uBumpTexture, samplePositionTexspace.xy).r;
	}

	return samplePositionTexspace.xy;
}

void main()
{
	vec3 T = normalize(vTangent);		// tangent
	vec3 B = normalize(vBitangent);		// bitangent
    vec3 N = normalize(vNormal);		// normal

	// Convert normal from tangent space to world space
	mat3 TBN = mat3(T, B, N);
	vec2 texCoords = vTexCoord;

	// Relief map
	if (noBump == 0.0)
		texCoords = ReliefMapping(vTexCoord, TBN);

	// Normal map
	if (noNormal == 0.0)
	{
		vec3 tangentSpaceNormal = texture(uNormalMap, texCoords).xyz * 2.0 - vec3(1.0);
		N = TBN * tangentSpaceNormal;
	}

	oNormals = vec4(N, 1.0);
	oAlbedo = texture(uTexture, texCoords);

	if(noTexture == 1.0)
		oAlbedo = vec4(0.5);

	float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far for demonstration
	oDepth = vec4(vec3(depth), 1.0);

	oPosition = vec4(vec3(vPosition), 1.0);
}

#endif
#endif

#ifdef LIGHTING_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 ViewPos; // in worldspace

void main()
{
	vTexCoord = aTexCoord;
	ViewPos = vec3(uCameraPosition);

	gl_Position =  vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 ViewPos;		// in worldspace

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform sampler2D oNormals;
uniform sampler2D oAlbedo;
uniform sampler2D oDepth;
uniform sampler2D oPosition;

layout(location = 0) out vec4 oColor;

void main()
{
    // G buffer
	vec3 vposition = texture(oPosition, vTexCoord).rgb;
	vec3 Normal = texture(oNormals, vTexCoord).rgb;
	vec3 albedo = texture(oAlbedo, vTexCoord).rgb;
	vec3 viewDir = normalize((ViewPos - vposition));

	// Mat parameters
    vec3 specular = vec3(1.0); // color reflected by mat
    float shininess = 120.0; // how strong specular reflections are (more shininess harder and smaller spec)

	// Ambient
    float ambientIntensity = 0.4;
    vec3 ambientColor = albedo.xyz * ambientIntensity;

	// Note: This was being normalized again and gave a bug
    vec3 N = Normal; // normal

	vec3 finalDiffuse;
	vec3 finalSpecular;

	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 1.0f;
		vec3 L;
		vec3 diffuseColor;
		vec3 specularColor;
		// --- If we have a point light, attenuate according to distance ---
		if(uLight[i].type == 1)
		{
			float dist = length(uLight[i].position - vposition);
			float linear = 0.05;
			float quadratic = 0.01;
			attenuation = 1.0 / (1.0 + linear * dist + quadratic * dist * dist);
			L = normalize(uLight[i].position - vposition); // Light direction 
		}
	    else    
			L = normalize(uLight[i].direction); // Light direction 

	    vec3 R = reflect(-L, N); // reflected vector
	    
	    // Diffuse
	    float diffuseIntensity = max(dot(N, L), 0.0);
	    diffuseColor += albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	    // Specular
		float specularIntensity = pow(max(dot(viewDir, R), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;

		diffuseColor*=attenuation;
		specularColor*=attenuation;
	
		finalDiffuse += diffuseColor;
		finalSpecular += specularColor;
	}

	finalDiffuse /= uLightCount;
	finalSpecular /= uLightCount;

	// Final outputs
    oColor = vec4(ambientColor + finalDiffuse + finalSpecular, 1.0);
}

#endif
#endif

#ifdef BLOOM_BRIGHTEST

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position =  vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform sampler2D colorTexture;
uniform float threshold;
in vec2 vTexCoord;
out vec4 outColor;

void main()
{
    vec4 color = texture(colorTexture, vTexCoord);
	float intensity = dot(color.rgb, vec3(0.21, 0.71, 0.08));
	//float threshold = 0.6;
	float threshold1 = threshold;
	float threshold2 = threshold + 0.1;
	outColor = color * smoothstep(threshold1, threshold2, intensity);
}

#endif
#endif

#ifdef BLOOM_BLUR

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position =  vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform sampler2D colorMap;
uniform vec2 direction;
uniform int inputLod;
uniform int kernelRadius;

in vec2 vTexCoord;
out vec4 outColor;

void main()
{
    vec2 texSize = textureSize(colorMap, inputLod);
	vec2 texelSize = 1.0/texSize;
	vec2 margin1 = texelSize * 0.5;
	vec2 margin2 = vec2(1.0) - margin1;

	outColor = vec4(0.0);

	vec2 directionFragCoord = gl_FragCoord.xy * direction;
	int coord = int(directionFragCoord.x + directionFragCoord.y);
	vec2 directionTexSize = texSize * direction;
	int size = int(directionTexSize.x + directionTexSize.y);
	//int kernelRadius = 24;
	int kernelBegin = -min(kernelRadius, coord);
	int kernelEnd = min(kernelRadius, size - coord);
	float weight = 0.0;

	for(int i = kernelBegin; i <= kernelEnd; ++i)
	{
		float currentWeight = smoothstep(float(kernelRadius), 0.0, float(abs(i)));
		vec2 finalTexCoords = vTexCoord + i * direction * texelSize;
	    finalTexCoords = clamp(finalTexCoords, margin1, margin2);
		outColor += textureLod(colorMap, finalTexCoords, inputLod) * currentWeight;
		weight += currentWeight;		
	}

	outColor = outColor / weight;
}

#endif
#endif

#ifdef BLOOM

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position =  vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform sampler2D colorMap;
uniform int maxLod;
uniform int lodI0;
uniform int lodI1;
uniform int lodI2;
uniform int lodI3;
uniform int lodI4;

in vec2 vTexCoord;
out vec4 outColor;

void main()
{  
	outColor = vec4(0.0);
	for(int lod = 0; lod < maxLod; ++lod)
	{
		if(lod == 0)
			outColor += textureLod(colorMap, vTexCoord, float(lod)) * lodI0;
		else if (lod == 1)
			outColor += textureLod(colorMap, vTexCoord, float(lod)) * lodI1;
		else if (lod == 2)
			outColor += textureLod(colorMap, vTexCoord, float(lod)) * lodI2;
		else if (lod == 3)
			outColor += textureLod(colorMap, vTexCoord, float(lod)) * lodI3;
		else if (lod == 4)
			outColor += textureLod(colorMap, vTexCoord, float(lod)) * lodI4;
	}
	outColor.a = 1.0;
}

#endif
#endif

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.