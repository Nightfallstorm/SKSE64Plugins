SamplerState SampleType;

struct LayerData
{
	uint blendMode;
	uint type;
	float4 color;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

Texture2D sourceTexture : register(t0);
Texture2D blendTexture : register(t1);
StructuredBuffer<LayerData> layerData : register(t2);

float BlendTintf(float a, float b)
{
    float r;
    if(a < 0.5) r = 2.0 * a * b; else r = 1.0 - 2.0 * (1.0 - b) * (1.0 - a);
    return r;
}

float4 Blend(float4 a, float4 b)
{
	float4 r;
	b.rgb = clamp(b.rgb / b.a, 0, 1);
	r.r = BlendTintf(a.r, b.r);
	r.g = BlendTintf(a.g, b.g);
	r.b = BlendTintf(a.b, b.b);
	r.rgb = (1 - b.a) * a.rgb + r.rgb * b.a;
	r.a = a.a;
	return r;
}

// Technique name must be lowercase
float4 tint(PixelInputType input) : SV_TARGET
{
	float4 source = sourceTexture.Sample(SampleType, input.tex);
	float4 layer = blendTexture.Sample(SampleType, input.tex);
	
	switch(layerData[0].type)
	{
		case 0:
		layer = layer * layerData[0].color;
		break;
		case 1:
		layer = float4(layerData[0].color.rgb, layer.r * layerData[0].color.a);
		break;
		case 2:
		layer = layerData[0].color;
		break;
	}

	return Blend(source, layer);
}