struct cVertIn
{
	float4 position : POSITION;
	float2 texcoord0 : TEXCOORD0;
	float3 texcoord1 : TEXCOORD1;
	float4 color : COLOR;
};

struct cVertOut
{
	float4 position : POSITION;
};

extern uniform float4x4 modelToClip;

cVertOut main(cVertIn In)
{
	cVertOut Out;
	Out.position = mul(In.position, modelToClip);
	return Out;
}