struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D diffuseTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
extern uniform float4 customParams[2];

float4 main(cVertOut In) : COLOR
{
	float4 diffuseTexColor = tex2D(diffuseTexture, In.texcoord1.xy);
	float3 mixedColor = lerp(diffuseTexColor.rgb, customParams[0].rgb, customParams[0].a);
	mixedColor = lerp(mixedColor, customParams[1].rgb, customParams[1].a * diffuseTexColor.a);
	float alpha = tex2D(tintMaskTexture, In.texcoord1.xy).g;
	return float4(mixedColor, alpha);
}