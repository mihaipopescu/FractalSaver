#include "D3DScreensaver.h"


//-----------------------------------------------------------------------------
// Name : CD3DScreensaver() (Protected)
// Desc : An instance cannot be created by user using the c-tor.
//-----------------------------------------------------------------------------
CD3DScreensaver::CD3DScreensaver()
{
	m_hWnd = NULL;
	m_pD3D = NULL;
	m_pD3DDevice = NULL;
	ZeroMemory( &m_D3DPresentParams, sizeof(D3DPRESENT_PARAMETERS) );
	m_bLostDevice = false;
}

CD3DScreensaver::~CD3DScreensaver()
{
	DestroyDirect3D();
}

//-----------------------------------------------------------------------------
// Name : InitDirect3D () (Private)
// Desc : Performs a simple, non-enumerated, initialization of Direct3D
//-----------------------------------------------------------------------------
bool CD3DScreensaver::InitDirect3D(HWND hWnd)
{
    HRESULT               hRet;
    D3DPRESENT_PARAMETERS PresentParams;
    D3DCAPS9              Caps;
	D3DDISPLAYMODE        CurrentMode;
	RECT rect;

	m_hWnd = hWnd;

    // First of all create our D3D Object
    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
    if (!m_pD3D) 
    {
        MessageBox( m_hWnd, "No compatible Direct3D object could be created.", "Fatal Error!", MB_OK | MB_ICONSTOP | MB_APPLMODAL );
        return false;
    
    } // End if failure

    // Fill out a simple set of present parameters
    ZeroMemory( &PresentParams, sizeof(D3DPRESENT_PARAMETERS) );

    // Select back buffer format etc
	m_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &CurrentMode);
	PresentParams.BackBufferFormat = CurrentMode.Format;
    
	// get window dimensions & init BB
	GetClientRect(m_hWnd, &rect);
	m_nViewWidth = rect.right - rect.left;
	m_nViewHeight = rect.bottom - rect.top;
	PresentParams.BackBufferWidth = m_nViewWidth;
	PresentParams.BackBufferHeight = m_nViewHeight;
        
	// Setup remaining flags
	PresentParams.AutoDepthStencilFormat = FindDepthStencilFormat( D3DADAPTER_DEFAULT, CurrentMode, D3DDEVTYPE_HAL );
    PresentParams.SwapEffect			 = D3DSWAPEFFECT_DISCARD;
	PresentParams.EnableAutoDepthStencil = true;
	
	// Set Creation Flags
	unsigned long ulFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    // Check if Hardware T&L is available
    ZeroMemory( &Caps, sizeof(D3DCAPS9) );
    m_pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &Caps );
    if ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) ulFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
        
    // Attempt to create a HAL device
    if( FAILED( hRet = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, ulFlags, &PresentParams, &m_pD3DDevice ) ) ) 
    {
        MessageBox( m_hWnd, "Could not create a valid HAL Direct3D device object.\r\n\r\n"
                            "The system will now attempt to create a device utilising the 'Reference Rasterizer' (D3DDEVTYPE_REF)",
                            "Fatal Error!", MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
        
        // Find REF depth buffer format
        PresentParams.AutoDepthStencilFormat = FindDepthStencilFormat( D3DADAPTER_DEFAULT, CurrentMode, D3DDEVTYPE_REF );

        // Check if Hardware T&L is available
        ZeroMemory( &Caps, sizeof(D3DCAPS9) );
        ulFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        m_pD3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, &Caps );
        if ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) ulFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

        // Attempt to create a REF device
        if( FAILED( hRet = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, m_hWnd, ulFlags, &PresentParams, &m_pD3DDevice ) ) ) 
        {
            MessageBox( m_hWnd, "Could not create a valid REF Direct3D device object.\r\n\r\nThe system will now exit.",
                                "Fatal Error!", MB_OK | MB_ICONSTOP | MB_APPLMODAL );

            // Failed
            return false;
        
        } // End if Failure (REF)

    } // End if Failure (HAL)

    // Store the present parameters
    m_D3DPresentParams = PresentParams;

	// setups initial states
	SetupRenderStates();

    // Success!!
    return true;
}

//-----------------------------------------------------------------------------
// Name : FindDepthStencilFormat ()
// Desc : This function simply determines the best depth format that is
//        available for the specified mode.
// Note : No tests for stencil active depth buffers are made.
//-----------------------------------------------------------------------------
D3DFORMAT CD3DScreensaver::FindDepthStencilFormat( ULONG AdapterOrdinal, D3DDISPLAYMODE Mode, D3DDEVTYPE DevType )
{

    // Test for 24 bith depth buffer
    if (SUCCEEDED( m_pD3D->CheckDeviceFormat(AdapterOrdinal, DevType, Mode.Format, D3DUSAGE_DEPTHSTENCIL , D3DRTYPE_SURFACE , D3DFMT_D32 )))
    {
        if (SUCCEEDED( m_pD3D->CheckDepthStencilMatch ( AdapterOrdinal, DevType, Mode.Format, Mode.Format, D3DFMT_D32 ))) return D3DFMT_D32;
    
    } // End if 32bpp Available

    // Test for 24 bit depth buffer
    if (SUCCEEDED( m_pD3D->CheckDeviceFormat(AdapterOrdinal, DevType, Mode.Format, D3DUSAGE_DEPTHSTENCIL , D3DRTYPE_SURFACE , D3DFMT_D24X8 )))
    {
        if (SUCCEEDED( m_pD3D->CheckDepthStencilMatch ( AdapterOrdinal, DevType, Mode.Format, Mode.Format, D3DFMT_D24X8 ))) return D3DFMT_D24X8;
    
    } // End if 24bpp Available

    // Test for 16 bit depth buffer
    if (SUCCEEDED( m_pD3D->CheckDeviceFormat(AdapterOrdinal, DevType, Mode.Format, D3DUSAGE_DEPTHSTENCIL , D3DRTYPE_SURFACE , D3DFMT_D16 )))
    {
        if (SUCCEEDED( m_pD3D->CheckDepthStencilMatch ( AdapterOrdinal, DevType, Mode.Format, Mode.Format, D3DFMT_D16 ))) return D3DFMT_D16;
    
    } // End if 16bpp Available

    // No depth buffer supported
    return D3DFMT_UNKNOWN;
}

void CD3DScreensaver::DestroyDirect3D()
{
    // Destroy Direct3D Objects
    if ( m_pD3DDevice ) m_pD3DDevice->Release();
    if ( m_pD3D       ) m_pD3D->Release();

    m_pD3D       = NULL;
    m_pD3DDevice = NULL;
}

//-----------------------------------------------------------------------------
// Name : SetupRenderStates ()
// Desc : Sets up all the initial states required by the renderer.
//-----------------------------------------------------------------------------
void CD3DScreensaver::SetupRenderStates()
{
    // Setup our D3D Device initial states
    m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	m_pD3DDevice->SetRenderState( D3DRS_DITHERENABLE, FALSE );
	m_pD3DDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    m_pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    // Setup our vertex FVF code
	m_pD3DDevice->SetFVF( sScreenVertex::FVF_FLAGS );
}

//-----------------------------------------------------------------------------
// Name : FrameAdvance
// Desc : Renders the current frame
//-----------------------------------------------------------------------------
void CD3DScreensaver::FrameAdvance()
{
	// Recover lost device if required
    if ( m_bLostDevice )
    {
        // Can we reset the device yet ?
        HRESULT hRet = m_pD3DDevice->TestCooperativeLevel();
        if ( hRet == D3DERR_DEVICENOTRESET )
        {
            // Restore the device
            m_pD3DDevice->Reset( &m_D3DPresentParams );
            SetupRenderStates( );
            m_bLostDevice = false;
        
        } // End if can reset
        else
        {
            return;
        } // End if cannot reset

    } // End if Device Lost

	// Clear the frame & depth buffer ready for drawing
    m_pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0 );
    
    // Begin Scene Rendering
    m_pD3DDevice->BeginScene();

	// render specific stuff
	Render();

	// End Scene Rendering
    m_pD3DDevice->EndScene();
    
    // Present the buffer
	if ( FAILED(m_pD3DDevice->Present( NULL, NULL, NULL, NULL )) ) m_bLostDevice = true;
}