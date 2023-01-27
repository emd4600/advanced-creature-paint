struct cVertOut
{
	float4 position : POSITION;
};

extern uniform float4 customParams;

float4 main(cVertOut In) : COLOR
{
	return customParams;
}