struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D partDiffuseTexture : register(s0);

float4 main(cVertOut In) : COLOR
{
	return tex2D(partDiffuseTexture, In.texcoord1.xy);
}
