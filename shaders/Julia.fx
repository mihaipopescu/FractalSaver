struct VS_INPUT
{
	float3 pos : POSITION0;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : POSITION0;
	float2 tex : TEXCOORD0;
};

const int MAX_ITERATIONS = 128;
const float2 JuliaSeed = float2(0.39, -0.2);
uniform float AspectRatio = 1.0;
uniform float ZoomFactor = 3;
uniform float2 Pan = float2(0, 0);

texture Palette  <
string ResourceName = "";//Optional default file name
string UIName =  "Palette Texture";
string ResourceType = "2D";
>;

sampler2D PaletteSampler = sampler_state {
	Texture = <Palette>;
	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
};

VS_OUTPUT mainVS(VS_INPUT IN)
{
	VS_OUTPUT OUT = (VS_OUTPUT)0;
	OUT.pos = float4(IN.pos.xyz, 1);
	OUT.tex = (IN.tex - 0.5) * ZoomFactor * float2(AspectRatio, 1) - Pan;
	return OUT;
}

float IterateFractal(float2 z, float2 c)
{
	float v = 1.0f;
	int it = 0, lastIT = MAX_ITERATIONS;
	do
	{
		float x2 = z.x * z.x;
		float y2 = z.y * z.y;
		
		if( lastIT == MAX_ITERATIONS && x2 + y2 > 4.0 )
			lastIT = it+1;
		
		//z[n+1] = z[n]^2 + C
		z.y = 2 * z.x * z.y + c.y;
		z.x = x2 - y2 + c.x;
		
		++it;
	}
	while(it < lastIT);
	
	return float(it)/float(MAX_ITERATIONS);
}

float4 JuliaPS_EscapeTimePS(float2 z : TEXCOORD0 ) : COLOR0
{
	float v = IterateFractal(z, JuliaSeed);
	
	return float4(v,v,v,1);
}

float4 MandelPS_EscapeTimePS(float2 z : TEXCOORD0) : COLOR0
{
	float v = IterateFractal(z, z);
	
	return float4(v,v,v,1);
}

technique Julia
{
	pass p0 
	{
		VertexShader = compile vs_3_0 mainVS();
		PixelShader = compile ps_3_0 JuliaPS_EscapeTimePS();
	}
}

technique Mandelbrot
{
	pass p0 
	{
		VertexShader = compile vs_3_0 mainVS();
		PixelShader = compile ps_3_0 MandelPS_EscapeTimePS();
	}
}
