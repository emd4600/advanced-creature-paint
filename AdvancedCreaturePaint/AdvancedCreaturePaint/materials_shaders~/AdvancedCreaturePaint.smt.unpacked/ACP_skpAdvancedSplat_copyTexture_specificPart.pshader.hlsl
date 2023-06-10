struct cVertOut
{
	float4 position : POSITION;
	float2 texcoord0 : TEXCOORD0;
};

sampler2D copyTexture : register(s0);

float4 main(cVertOut In) : COLOR
{
	return tex2D(copyTexture, In.texcoord0);
}
