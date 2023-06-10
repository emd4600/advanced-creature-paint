struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D normalSpecTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
extern uniform struct {
	float4 normalStrength;
	float4 specularStrength;
	// r=identity, 1.0 if applied 0.0 if not
	float4 applyIdentityColor;
} customParams;

float4 main(cVertOut In) : COLOR
{
	float4 normalSpec = tex2D(normalSpecTexture, In.texcoord1.xy);
	float4 tintMask = tex2D(tintMaskTexture, In.texcoord1.xy);
	float4 maskNormal = tintMask * customParams.normalStrength;
	float4 maskSpec = tintMask * customParams.specularStrength;
	
	// We want to "weaken" these colors acoording to the tintMask
	// Tipically, blending would be:
	// color = tintMask.r*baseColor + (1 - tintMask.r)*color
	//
	// color = tintMask.g*coatColor + (1 - tintMask.g)*color =
	//       = tintMask.g*coatColor + (1 - tintMask.g)*(tintMask.r*baseColor + (1 - tintMask.r)*color) = 
	//       = tintMask.g*coatColor + (1 - tintMask.g)*tintMask.r*baseColor + (1 - tintMask.g)*(1 - tintMask.r)*color
	//
	// color = tintMask.b*detailColor + (1 - tintMask.b)*color = 
	//       = tintMask.b*detailColor + (1 - tintMask.b)*tintMask.g*coatColor + (1 - tintMask.b)*(1 - tintMask.g)*tintMask.r*baseColor + (1 - tintMask.b)*(1 - tintMask.g)*(1 - tintMask.r)*color

	float3 normal = maskNormal.b*normalSpec.xyz;
	float spec = maskSpec.b*normalSpec.a;
	
	if (customParams.applyIdentityColor.r > 0.5) {
		normal = normal*(1 - maskNormal.a);
		spec = spec*(1 - maskSpec.a);
	}
	
	return float4(normal, spec);
}