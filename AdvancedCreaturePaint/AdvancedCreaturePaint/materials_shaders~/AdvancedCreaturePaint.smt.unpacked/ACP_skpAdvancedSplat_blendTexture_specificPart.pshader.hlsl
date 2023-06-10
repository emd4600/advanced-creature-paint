struct cVertOut
{
	float4 position : POSITION;
	float2 texcoord0 : TEXCOORD0;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D diffuseTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
extern uniform struct {
	float4 tintMaskMask;
} customParams;

float4 main(cVertOut In) : COLOR
{
	float3 color = tex2D(diffuseTexture, In.texcoord0.xy).rgb;
	float4 mask = tex2D(tintMaskTexture, In.texcoord1.xy);
	float maskValue = dot(mask, customParams.tintMaskMask);
	return float4(color, maskValue);
}
