#pragma once
#include "D3DScreensaver.h"


class CFractalSaver : public CD3DScreensaver
{
public:
	// Singleton
	static CFractalSaver*	GetInstance() { return m_psoInstance; }
	static void				CreateInstance() { delete m_psoInstance; m_psoInstance = new CFractalSaver(); }
	static void				DestroyInstance() { delete m_psoInstance; }

	bool					CreateRenderObjects();
	virtual void			Render();

protected:
							CFractalSaver();
	virtual					~CFractalSaver(void);
	void					DestroyRenderObjects();

protected:
	LPD3DXEFFECT			m_pFractalEffect;
	LPDIRECT3DTEXTURE9		m_pLUT;
	LPDIRECT3DVERTEXBUFFER9 m_pFullScreenQuadVB;

	static CFractalSaver*	m_psoInstance;
};
