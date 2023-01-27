struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D tintMaskTexture;
extern uniform float4 customParams;

float4 main(cVertOut In) : COLOR
{
	float alpha = tex2D(tintMaskTexture, In.texcoord1.xy).w;
	return float4(customParams.xyz, alpha);
}