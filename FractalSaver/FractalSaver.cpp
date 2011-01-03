#include "FractalSaver.h"

CFractalSaver* CFractalSaver::m_psoInstance = NULL;


CFractalSaver::CFractalSaver()
{
	m_pFullScreenQuadVB = NULL;
}

CFractalSaver::~CFractalSaver(void)
{
	DestroyRenderObjects();
}

bool CFractalSaver::CreateRenderObjects()
{
	sScreenVertex quad[4] = {
		{-1.f, -1.f, 0.0f, 0.f, 1.f},
		{1.0f, -1.f, 0.0f, 1.f, 1.f},
		{1.0f, 1.0f, 0.0f, 1.f, 0.f},
		{-1.f, 1.0f, 0.0f, 0.f, 0.f} };

	if( FAILED( m_pD3DDevice->CreateVertexBuffer( sizeof(sScreenVertex) * 4, D3DUSAGE_WRITEONLY, sScreenVertex::FVF_FLAGS, D3DPOOL_DEFAULT, &m_pFullScreenQuadVB, NULL) ) )
		return false;

	sScreenVertex* pVertex = NULL;

	// Lock the vertex buffer ready to fill data
	if( FAILED( m_pFullScreenQuadVB->Lock( 0, sizeof(sScreenVertex) * 4, (void**)&pVertex, 0 ) ) )
		return false;

	// Copy over the vertex data
	memcpy( pVertex, quad, sizeof(sScreenVertex) * 4 );

	// We are finished with the vertex buffer
	m_pFullScreenQuadVB->Unlock();

	LPD3DXBUFFER pBufferErrors = NULL;
	HRESULT hr = D3DXCreateEffectFromFile( m_pD3DDevice, "shaders/julia.fx", NULL, NULL, 0, NULL, &m_pFractalEffect, &pBufferErrors );

	// If there are errors, notify the users
	if( FAILED( hr ) )
	{
		char hrs[10];
		_ltoa_s(hr, hrs, 10, 16);
		OutputDebugStr("FATAL ERROR CODE=0x");
		OutputDebugStr(hrs);
		OutputDebugStr(". ");
		if( pBufferErrors )
			OutputDebugStr( (LPSTR) pBufferErrors->GetBufferPointer() );
		OutputDebugStr("\n");
		return false;
	}

	float fAspr = (float)m_nViewWidth / (float)m_nViewHeight;

	m_pFractalEffect->SetFloat("AspectRatio", fAspr);
	m_pFractalEffect->SetTechnique("Mandelbrot");

	return true;
}

void CFractalSaver::DestroyRenderObjects()
{
	if( m_pFullScreenQuadVB )
		m_pFullScreenQuadVB->Release();
}

void CFractalSaver::Render()
{
	UINT uiNumPasses = 0;
	m_pFractalEffect->Begin( &uiNumPasses, 0 );

	for( UINT iPass = 0; iPass < uiNumPasses; ++iPass )
	{
		m_pFractalEffect->BeginPass( iPass );

		m_pD3DDevice->SetStreamSource(0, m_pFullScreenQuadVB, 0, sizeof(sScreenVertex));
		m_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

		m_pFractalEffect->EndPass();
	}

	m_pFractalEffect->End();
}