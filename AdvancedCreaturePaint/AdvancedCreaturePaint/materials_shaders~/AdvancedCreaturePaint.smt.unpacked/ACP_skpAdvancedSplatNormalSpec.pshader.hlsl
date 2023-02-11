struct cVertOut
{
	float4 position : POSITION;
	float2 texcoord0 : TEXCOORD0;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D sourceNormSpecTexture : register(s0);
sampler2D rigblockDiffuseTexture : register(s1);
sampler2D tintMaskTexture : register(s2);
// These are normalSpec textures from the paints
sampler2D baseTexture : register(s3);
sampler2D coatTexture : register(s4);
sampler2D detailTexture : register(s5);
sampler2D identityTexture : register(s6);
sampler2D texturedTexture : register(s7);
extern uniform struct {
	// base, coat, detail, identity
	float4 applyColor0;
	// textured, ?, ?, ?
	float4 applyColor1;
} customParams;

float4 main(cVertOut In) : COLOR
{
	float4 diffuseTexture = tex2D(rigblockDiffuseTexture, In.texcoord1.xy);
	float4 mask = tex2D(tintMaskTexture, In.texcoord1.xy);
	
	float4 baseNormSpec = tex2D(baseTexture, In.texcoord1.xy);
	float4 coatNormSpec = tex2D(coatTexture, In.texcoord1.xy);
	float4 detailNormSpec = tex2D(detailTexture, In.texcoord1.xy);
	float4 identityNormSpec = tex2D(identityTexture, In.texcoord1.xy);
	float4 texturedNormSpec = tex2D(texturedTexture, In.texcoord1.xy);
	
	// This texture is for the whole creature, so we use different UV coords
	float4 sourceNormSpec = tex2D(sourceNormSpecTexture, In.texcoord0);
	
	float4 colorDiff, colorMix;
	if (customParams.applyColor0.r > 0.0 &&
	    customParams.applyColor1.r > 0.0)
	{
		colorMix = lerp(baseNormSpec, texturedNormSpec, diffuseTexture.a);
	}
	else if (customParams.applyColor0.r > 0.0)
	{
		colorMix = lerp(baseNormSpec, sourceNormSpec, diffuseTexture.a);
	}
	else
	{
		colorMix = lerp(sourceNormSpec, texturedNormSpec, diffuseTexture.a);
	}
	// Apply base color
	colorDiff = baseNormSpec - colorMix;
	colorMix  = customParams.applyColor0.r * mask.x * colorDiff + colorMix;
	// Apply coat color
	colorDiff = coatNormSpec - colorMix;
	colorMix  = customParams.applyColor0.g * mask.y * colorDiff + colorMix;
	// Apply detail color
	colorDiff = detailNormSpec - colorMix;
	colorMix  = customParams.applyColor0.b * mask.z * colorDiff + colorMix;
	// Apply identity color
	colorDiff = identityNormSpec - colorMix;
	colorMix  = customParams.applyColor0.a * mask.a * colorDiff + colorMix;
	
	return colorMix;
}
