struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

/*sampler2D rigblockDiffuseTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
sampler2D baseTexture : register(s2);
sampler2D coatTexture : register(s3);
sampler2D detailTexture : register(s4);
sampler2D identityTexture : register(s5);
extern uniform struct {
	float4 baseColor0;
	float4 baseColor1;
	float4 coatColor0;
	float4 coatColor1;
	float4 detailColor0;
	float4 detailColor1;
	float4 identityColor0;
	float4 identityColor1;
} customParams;

float4 main(cVertOut In) : COLOR
{
	float4 rigblockDiffuse = tex2D(rigblockDiffuseTexture, In.texcoord1.xy);
	
	float baseColorBrightness = dot(customParams.baseColor0.rgb, float3(0.3, 0.6, 0.1));
	float rigblockBrightness = dot(rigblockDiffuse.rgb, float3(0.3, 0.6, 0.1));
	
	float scale = lerp(0.98, 1.0, rigblockBrightness / (baseColorBrightness + 0.001));
	
	float4 tintMask = tex2D(tintMaskTexture, In.texcoord1.xy);
	
	float3 dstColor;
	dstColor = scale * customParams.baseColor0.rgb - rigblockDiffuse.rgb;
	dstColor = tintMask.x * dstColor + rigblockDiffuse.rgb;
	rigblockDiffuse.rgb = scale * customParams.coatColor0 - dstColor;
	dstColor = tintMask.y * rigblockDiffuse.rgb + dstColor;
	rigblockDiffuse.rgb = scale * customParams.detailColor0 - dstColor;
	dstColor = tintMask.z * rigblockDiffuse.rgb + dstColor;
	
	return float4(dstColor, 1.0);
}*/

sampler2D rigblockDiffuseTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
sampler2D baseTexture : register(s2);
sampler2D coatTexture : register(s3);
sampler2D detailTexture : register(s4);
sampler2D identityTexture : register(s5);
extern uniform struct {
	float4 baseColor0;
	float4 baseColor1;
	float4 coatColor0;
	float4 coatColor1;
	float4 detailColor0;
	float4 detailColor1;
	float4 identityColor0;
	float4 identityColor1;
} customParams;

float3 mixPaintColor(cVertOut In, sampler2D inputTexture, float4 color0, float4 color1)
{
	float4 diffuseTexColor = tex2D(inputTexture, In.texcoord1.xy);
	float3 mixedColor = lerp(diffuseTexColor.rgb, color0.rgb, color0.a);
	return lerp(mixedColor, color1.rgb, color1.a * diffuseTexColor.a);
}

float4 main(cVertOut In) : COLOR
{
	float4 clr = tex2D(rigblockDiffuseTexture, In.texcoord1.xy);
	float4 mask = tex2D(tintMaskTexture, In.texcoord1.xy);
	
	float3 baseColor = mixPaintColor(In, baseTexture, customParams.baseColor0, customParams.baseColor1);
	float3 coatColor = mixPaintColor(In, coatTexture, customParams.coatColor0, customParams.coatColor1);
	float3 detailColor = mixPaintColor(In, detailTexture, customParams.detailColor0, customParams.detailColor1);

	//float baseBrightness = dot(baseColor, float3(0.3, 0.6, 0.1));
	//float rigblockBrightness = dot(clr.rgb, float3(0.3, 0.6, 0.1));
	//float scale = lerp(1.0, rigblockBrightness / (baseBrightness + 0.001), 0.98);

	float scale = 1.0;
	float3 dstColor;
	dstColor = scale * baseColor - clr.rgb;
	dstColor = mask.x * dstColor + clr.rgb;
	clr.rgb = scale * coatColor - dstColor;
	dstColor = mask.y * clr.rgb + dstColor;
	clr.rgb = scale * detailColor - dstColor;

	float4 outputColor;
	outputColor.a = clr.a;
	outputColor.rgb = mask.z * clr.rgb + dstColor;
	//outputColor.rgb = tex2D(baseTexture, In.texcoord1.xy);
	//outputColor.rgb = baseColor;
	return outputColor;
}
