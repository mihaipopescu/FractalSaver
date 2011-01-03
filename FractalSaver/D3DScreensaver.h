#pragma once
#include "D3DX9.h"

struct sScreenVertex
{
	float x, y, z;
	float u, v;
	enum FVF
	{
		FVF_FLAGS = D3DFVF_XYZ | D3DFVF_TEX1
	};
};

class CD3DScreensaver
{
public:
	bool					InitDirect3D(HWND hWnd);
	void					FrameAdvance();

protected:
							CD3DScreensaver(void);
	virtual					~CD3DScreensaver(void);

	void					SetupRenderStates();
	void					DestroyDirect3D();
	D3DFORMAT				FindDepthStencilFormat( ULONG AdapterOrdinal, D3DDISPLAYMODE Mode, D3DDEVTYPE DevType );

	virtual void			Render() { ; }

protected:
	HWND					m_hWnd;
	IDirect3D9*				m_pD3D;
	IDirect3DDevice9*		m_pD3DDevice;
	D3DPRESENT_PARAMETERS	m_D3DPresentParams;
	bool					m_bLostDevice;

	ULONG                   m_nViewWidth;       // Width of render viewport
    ULONG                   m_nViewHeight;      // Height of render viewport
};
