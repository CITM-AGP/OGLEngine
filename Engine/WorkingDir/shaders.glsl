///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

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

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

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
out vec3 vPosition; // in worldspace
out vec3 vNormal; // in worldspace
out vec3 vViewDir; // in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);

	//float clippingScale = 5.0;

	//gl_Position = vec4(aPosition, clippingScale);

	//gl_Position.z = -gl_Position.z;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition; // in worldspace
in vec3 vNormal; // in worldspace
in vec3 uViewDir; // in worldspace

uniform sampler2D uTexture;

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

void main()
{
	// Mat parameters
    vec3 specular = vec3(1.0); // color reflected by mat
    float shininess = 40.0; // how strong specular reflections are (more shininess harder and smaller spec)
	vec4 albedo = texture(uTexture, vTexCoord);

	// Ambient
    float ambientIntensity = 0.4;
    vec3 ambientColor = albedo.xyz * ambientIntensity;

    vec3 N = normalize(vNormal); // normal
	vec3 V = normalize(-uViewDir.xyz); // direction from pixel to camera

	vec3 diffuseColor;
	vec3 specularColor;

	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 1.0f;
		
		// --- If we have a point light, attenuate according to distance ---
		if(uLight[i].type == 1)
			attenuation = 1.0 / length(uLight[i].position - vPosition);
	        
	    vec3 L = normalize(uLight[i].direction - uViewDir.xyz); // Light direction 
	    vec3 R = reflect(-L, N); // reflected vector
	    
	    // Diffuse
	    float diffuseIntensity = max(0.0, dot(N, L));
	    diffuseColor += attenuation * albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	    // Specular
	    float specularIntensity = pow(max(dot(R, V), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;
	}

	// Final outputs
	oColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);

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

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

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
out vec3 vPosition; // in worldspace
out vec3 vNormal; // in worldspace
out vec3 vViewDir; // in worldspace

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);

	//float clippingScale = 5.0;

	//gl_Position = vec4(aPosition, clippingScale);

	//gl_Position.z = -gl_Position.z;
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition; // in worldspace
in vec3 vNormal; // in worldspace
in vec3 uViewDir; // in worldspace

uniform sampler2D uTexture;
uniform int noTexture;

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

void main()
{
    oNormals = vec4(normalize(vNormal), 1.0); 
	oAlbedo = texture(uTexture, vTexCoord);

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
	ViewPos = vec3(uWorldViewProjectionMatrix * vec4(uCameraPosition, 1.0));
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
in vec3 ViewPos; // in worldspace

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
	vec3 viewDir = normalize(ViewPos - vposition);
	float Specular = texture(oAlbedo, vTexCoord).a;
	Specular = 1.0;
	// Mat parameters
    vec3 specular = vec3(1.0); // color reflected by mat
    float shininess = 40.0; // how strong specular reflections are (more shininess harder and smaller spec)

	// Ambient
    float ambientIntensity = 0.4;
    vec3 ambientColor = albedo.xyz * ambientIntensity;

    vec3 N = normalize(Normal); // normal
	//vec3 V = normalize(-viewDir.xyz); // direction from pixel to camera

	vec3 diffuseColor;
	vec3 specularColor;

	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 1.0f;
		vec3 L;
		// --- If we have a point light, attenuate according to distance ---
		if(uLight[i].type == 1)
		{
			float dist = length(uLight[i].position - vposition);
			float linear = 0.05;
			float quadratic = 0.01;
			attenuation = 1.0 / (1.0 + linear * dist + quadratic * dist * dist );
			L = normalize(uLight[i].position - vposition); // Light direction 
		}
	    else    
			L = normalize(uLight[i].direction); // Light direction 

	    vec3 R = reflect(-L, N); // reflected vector
	    
	    // Diffuse
	    float diffuseIntensity = max(dot(N, L), 0.0);
	    diffuseColor += albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	    // Specular
//		vec3 halfwayDir = normalize(L + viewDir);
//		float spec = pow(max(dot(N, halfwayDir), 0.0), shininess);
//		specularColor += uLight[i].color * spec * Specular;

		float specularIntensity = pow(max(dot(viewDir, R), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;

		diffuseColor*=attenuation;
		specularColor*=attenuation;

	}
	
	// Final outputs
    //oColor = vec4(Normal, 1.0);
    oColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);
	//oColor = vec4(ViewPos, 1.0);

}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.