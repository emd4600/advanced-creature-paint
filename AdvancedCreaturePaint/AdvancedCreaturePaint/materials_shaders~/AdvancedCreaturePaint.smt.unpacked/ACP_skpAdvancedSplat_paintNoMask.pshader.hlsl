struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D paintTexture : register(s0);
sampler2D partDiffuseTexture : register(s1);
extern uniform struct {
	float4 paintColor0;
	float4 paintColor1;
} customParams;

float3 mixPaintColor(cVertOut In, sampler2D inputTexture, float4 color0, float4 color1)
{
	float4 diffuseTexColor = tex2D(inputTexture, In.texcoord1.xy);
	float3 mixedColor = lerp(diffuseTexColor.rgb, color0.rgb, color0.a);
	return lerp(mixedColor, color1.rgb, color1.a * diffuseTexColor.a);
}

float4 main(cVertOut In) : COLOR
{
	float maskValue = tex2D(partDiffuseTexture, In.texcoord1.xy).a;
	
	float3 paintColor = mixPaintColor(In, paintTexture, customParams.paintColor0, customParams.paintColor1);

	return float4(paintColor, maskValue);
}
