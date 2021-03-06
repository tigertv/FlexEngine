#version 400

// Deferred PBR combine

// TODO: Move to common header
#define QUALITY_LEVEL_HIGH 1
#define NUM_POINT_LIGHTS 8
#define NUM_CASCADES 4

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

in vec2 ex_TexCoord;

out vec4 fragColor;

struct DirectionalLight 
{
	vec3 direction;
	int enabled;
	vec3 color;
	float brightness;
	int castShadows;
 	float shadowDarkness;
};
uniform DirectionalLight dirLight;

struct PointLight
{
	vec3 position;
	int enabled;
	vec3 color;
	float brightness;
};
uniform PointLight pointLights[NUM_POINT_LIGHTS];

uniform vec4 camPos;
uniform mat4 invView;
uniform mat4 invProj;
uniform bool enableIrradianceSampler;
// SSAO Sampling Data
uniform int enableSSAO = 1;
uniform float ssaoPowExp = 1.0f;
// Shadow Sampling Data
uniform mat4 cascadeViewProjMats[NUM_CASCADES];
uniform vec4 cascadeDepthSplits;
uniform float zNear;
uniform float zFar;

const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D normalRoughnessFrameBufferSampler;
layout (binding = 1) uniform sampler2D albedoMetallicFrameBufferSampler;
layout (binding = 2) uniform sampler2D ssaoBlurFrameBufferSampler;
layout (binding = 3) uniform sampler2D depthBuffer;
layout (binding = 4) uniform sampler2D brdfLUT;
layout (binding = 5) uniform sampler2DArray shadowMaps;
layout (binding = 6) uniform samplerCube irradianceSampler;
layout (binding = 7) uniform samplerCube prefilterMap;

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a2 = roughness * roughness;
	float NoH = max(dot(N, H), 0.0);
	float f = (NoH * a2 - NoH) * NoH + 1.0;
	return a2 / (PI * f * f);
}

float GeometrySchlickGGX(float NoV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NoV;
	float denom = NoV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(float NoV, float NoL, float roughness)
{
	float ggx2 = GeometrySchlickGGX(NoV, roughness);
	float ggx1 = GeometrySchlickGGX(NoL, roughness);

	return ggx1 * ggx2;
}

vec3 DoLighting(vec3 radiance, vec3 N, vec3 V, vec3 L, float NoV, float NoL,
	float roughness, float metallic, vec3 F0, vec3 albedo)
{
	vec3 H = normalize(V + L);

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(NoV, NoL, roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic; // Pure metals have no diffuse lighting

	vec3 nominator = NDF * G * F;
	float denominator = 4 * NoV * NoL + 0.001; // Add epsilon to prevent divide by zero
	vec3 specular = nominator / denominator;

	return (kD * albedo / PI + specular) * radiance * NoL;
}

// "UDN normal map blending": [Hill12]
// vec3 t = texture(baseMap,   uv).xyz * 2.0 - 1.0;
// vec3 u = texture(detailMap, uv).xyz * 2.0 - 1.0;
// vec3 r = normalize(t.xy + u.xy, t.z);
// return r;
//

vec3 ReconstructVSPosFromDepth(vec2 uv, float depth)
{
	float x = uv.x * 2.0f - 1.0f;
	float y = (1.0f - uv.y) * 2.0f - 1.0f;
	vec4 pos = vec4(x, y, depth, 1.0f);
	vec4 posVS = invProj * pos;
	vec3 posNDC = posVS.xyz / posVS.w;
	return posNDC;
}

vec3 ReconstructWSPosFromDepth(vec2 uv, float depth)
{
	float x = uv.x * 2.0f - 1.0f;
	float y = uv.y * 2.0f - 1.0f;
	vec4 pos = vec4(x, y, depth, 1.0f);
	vec4 posVS = invProj * pos;
	vec3 posNDC = posVS.xyz / posVS.w;
	return (invView * vec4(posNDC, 1)).xyz;
}

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

void main()
{
    float metallic = texture(albedoMetallicFrameBufferSampler, ex_TexCoord).a;

    vec3 N = texture(normalRoughnessFrameBufferSampler, ex_TexCoord).rgb;
    N = mat3(invView) * N; // view space -> world space
	// fragColor = vec4(N-.5+.5, 1.0);return;

    float ssao = enableSSAO == 1 ? texture(ssaoBlurFrameBufferSampler, ex_TexCoord).r : 1.0f;

    // fragColor = vec4(vec3(pow(ssao, ssaoPowExp)), 1); return;

    float roughness = texture(normalRoughnessFrameBufferSampler, ex_TexCoord).a;
    roughness = max(roughness, 0.045);

    float depth = texture(depthBuffer, ex_TexCoord).r;
    vec3 worldPos = ReconstructWSPosFromDepth(ex_TexCoord, depth);
    vec3 viewPos = ReconstructVSPosFromDepth(ex_TexCoord, depth);

    vec3 albedo = texture(albedoMetallicFrameBufferSampler, ex_TexCoord).rgb;

    float invDist = 1.0f/(zFar-zNear);
	float linDepth = (viewPos.z-zNear)*invDist;
	// fragColor = vec4(vec3(linDepth), 1); return;

	uint cascadeIndex = 0;
	for (uint i = 0; i < NUM_CASCADES; ++i)
	{
		if (linDepth > cascadeDepthSplits[i])
		{
			cascadeIndex = i + 1;
		}
	}

#if 0
	switch (cascadeIndex)
	{
		case 0: fragColor = vec4(1.0f, 0.2f, 0.0f, 0.0f); return;
		case 1: fragColor = vec4(0.0f, 1.0f, 0.2f, 0.0f); return;
		case 2: fragColor = vec4(0.0f, 0.2f, 1.0f, 0.0f); return;
		case 3: fragColor = vec4(0.1f, 0.1f, 0.2f, 0.0f); return;
	}
#endif

	vec3 V = normalize(camPos.xyz - worldPos);
	vec3 R = reflect(-V, N);

	float NoV = max(dot(N, V), 0.0);
	
	// If diaelectric, F0 should be 0.04, if metal it should be the albedo color
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Reflectance equation
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < NUM_POINT_LIGHTS; ++i)
	{
		if (pointLights[i].enabled == 0)
		{
			continue;
		}

		float distance = length(pointLights[i].position.xyz - worldPos);

		// This value still causes a visible harsh edge but our maps won't likely be 
		// this large and we'll have overlapping lights which will hide this
		if (distance > 125)
		{
			continue;
		}

		// Pretend point lights have a radius of 1cm to avoid division by 0
		float attenuation = 1.0 / max((distance * distance), 0.001);
		vec3 L = normalize(pointLights[i].position.xyz - worldPos);
		vec3 radiance = pointLights[i].color * pointLights[i].brightness * attenuation;
		float NoL = max(dot(N, L), 0.0);
	
		Lo += DoLighting(radiance, N, V, L, NoV, NoL, roughness, metallic, F0, albedo);
	}

	if (dirLight.enabled != 0)
	{
		vec3 L = normalize(dirLight.direction.xyz);
		vec3 radiance = dirLight.color * dirLight.brightness;
		float NoL = max(dot(N, L), 0.0);
		
		float dirLightShadowOpacity = 1.0;
		vec3 shadowMapTexelSize = 1.0 / textureSize(shadowMaps, 0);
		if (dirLight.castShadows != 0)
		{	
			vec4 transformedShadowPos = (biasMat * cascadeViewProjMats[cascadeIndex]) * vec4(worldPos, 1.0);

			float baseBias = 0.0005;
			float bias = max(baseBias * (1.0 - NoL), baseBias * 0.01);
			int sampleRadius = 5;
			float spread = 3.0;
			float shadowSampleContrib = dirLight.shadowDarkness / ((sampleRadius*2 + 1) * (sampleRadius*2 + 1));

#if QUALITY_LEVEL_HIGH
			for (int x = -sampleRadius; x <= sampleRadius; ++x)
			{
				for (int y = -sampleRadius; y <= sampleRadius; ++y)
				{
					float shadowDepth = texture(shadowMaps, 
						vec3(transformedShadowPos.xy + vec2(x, y) * shadowMapTexelSize.xy*spread, cascadeIndex)).r;

					if (shadowDepth > transformedShadowPos.z + bias)
					{
						dirLightShadowOpacity -= shadowSampleContrib;
					}
				}
			}
#else
			float shadowDepth = texture(shadowMaps, vec3(transformedShadowPos.xy, cascadeIndex)).r;
			if (shadowDepth > transformedShadowPos.z + bias)
			{
				dirLightShadowOpacity = 1.0 - dirLight.shadowDarkness;
			}
#endif
		}

		Lo += DoLighting(radiance, N, V, L, NoV, NoL, roughness, metallic, F0, albedo) * dirLightShadowOpacity;
	}

	vec3 F = FresnelSchlickRoughness(NoV, F0, roughness);

	vec3 ambient;
	if (enableIrradianceSampler)
	{
		// Diffse ambient term (IBL)
		vec3 kS = F;
	    vec3 kD = 1.0 - kS;
	    kD *= 1.0 - metallic;	  
	    vec3 irradiance = texture(irradianceSampler, N).rgb;
	    vec3 diffuse = irradiance * albedo;

		// Specular ambient term (IBL)
		const float MAX_REFLECTION_LOAD = 5.0;
		vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOAD).rgb;
		vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
		vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

		// Prevent specular light leaking [Russell15]
		float horizon = min(1.0 + dot(R, N), 1.0);
		specular *= horizon * horizon;

	    ambient = (kD * diffuse + specular);
	}
	else
	{
		ambient = vec3(0.03) * albedo;
	}

	vec3 color = ambient + Lo * pow(ssao, ssaoPowExp);

	// color = mix(color, vec3(
	// 	min(
	// 		mix(0.15, 0.2, max(abs(worldPos.x*0.5f),0.0)),
	// 		fract(worldPos.y*0.5+0.25)*0.5),
	// 	fract(worldPos.y*0.1)*0.2,
	// 	100.0f),
	// 		0.8f);

	fragColor = vec4(color, 1.0);
}
