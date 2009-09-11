#include"main.h"
#include"TaskSystemEx.h"

// アプリケーション関連

static HINSTANCE g_hInst=NULL;
HWND g_hWnd=NULL;

static RECT g_rcWnd;		// ウィンドウモード時の座標

static bool g_bWndMode=true;		// true：ウィンドウ / false：フルスクリーン
static bool g_bActive=false;		// アクティブ状態

// DirectX 関連

IDirect3D9 *g_pD3D=NULL;
IDirect3DDevice9 *g_pD3DDevice=NULL;
static D3DPRESENT_PARAMETERS g_D3DPP;
D3DPRESENT_PARAMETERS g_D3DPPWnd,g_D3DPPFull;

static bool g_bDeviceLost=false;		// デバイス状態

// デバッグ関連

static TCHAR g_debugInfo[32]=_T("計測中です");

// テクスチャ関連

static ID3DXSprite *g_pSprite=NULL;

// 描画
HRESULT Render(void)
{
	DWORD Flags=(g_D3DPP.EnableAutoDepthStencil==TRUE)?
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL : D3DCLEAR_TARGET;
	g_pD3DDevice->Clear(0,NULL,Flags,D3DCOLOR_ARGB(255,0,0,255),1.0f,0);

	if(SUCCEEDED(g_pD3DDevice->BeginScene()))
	{
		g_pSprite->Begin();
		TaskEx::DrawTask(g_pSprite);
		g_pSprite->End();

#ifdef _DEBUG
		IDirect3DSurface9 *pSurface;
		HDC hdc;

		g_pD3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pSurface);
		pSurface->GetDC(&hdc);

		TextOut(hdc,0,0,g_debugInfo,lstrlen(g_debugInfo));

		pSurface->ReleaseDC(hdc);
		pSurface->Release();
#endif
		g_pD3DDevice->EndScene();
	}

	return g_pD3DDevice->Present(NULL,NULL,NULL,NULL);
}

// D3D オブジェクト初期化
HRESULT InitD3DObject(void)
{
	g_pD3DDevice->SetRenderState(D3DRS_ZENABLE,
		g_D3DPP.EnableAutoDepthStencil ? D3DZB_TRUE:D3DZB_FALSE);

	g_pD3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,
		g_D3DPP.MultiSampleType!=D3DMULTISAMPLE_NONE ? TRUE:FALSE);

	g_pSprite->OnResetDevice();

	return S_OK;
}

// D3D オブジェクト解放
HRESULT CleanupD3DObject(void)
{
	g_pSprite->OnLostDevice();

	return S_OK;
}

// ウィンドウサイズ変更
HRESULT ChangeWindowSize(void)
{
	if(!g_pD3DDevice || g_bDeviceLost) return E_FAIL;

	CleanupD3DObject();

	HRESULT hr=g_pD3DDevice->Reset(&g_D3DPP);
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("ChangeWindowSize Reset"),hr);
        if(hr==D3DERR_DEVICELOST) g_bDeviceLost=true;
		else DestroyWindow(g_hWnd);
		return hr;
	}
	hr=InitD3DObject();
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("ChangeWindowSize InitD3DObject"),hr);
		DestroyWindow(g_hWnd);
		return hr;
	}

	D3DVIEWPORT9 vp;
	vp.X		= 0;
	vp.Y		= 0;
	vp.Width	= g_D3DPP.BackBufferWidth;
	vp.Height	= g_D3DPP.BackBufferHeight;
	vp.MinZ		= 0.0f;
	vp.MaxZ		= 1.0f;

	hr=g_pD3DDevice->SetViewport(&vp);
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("ChangeWindowSize SetViewport"),hr);
		DestroyWindow(g_hWnd);
		return hr;
	}

	return S_OK;
}

// WM_DISPLAYCHANGE メッセージ処理
void DisplayChange(void)
{
	if(g_bWndMode==false) return;

	DWORD x=g_rcWnd.left, y=g_rcWnd.top;
	DWORD w=g_rcWnd.right-g_rcWnd.left, h=g_rcWnd.bottom-g_rcWnd.top;

	SetWindowPos(g_hWnd,HWND_NOTOPMOST,x,y,w,h,SWP_DRAWFRAME);
}

// ディスプレイモード変更
HRESULT ChangeDisplayMode(void)
{
	if(g_bDeviceLost) return E_FAIL;

	g_bWndMode = !g_bWndMode;

	CleanupD3DObject();

	if(g_bWndMode)		// ウィンドウモードに
	{
		CopyMemory(&g_D3DPP,&g_D3DPPWnd,sizeof(D3DPRESENT_PARAMETERS));
	}
	else		// フルスクリーンモードに
	{
		CopyMemory(&g_D3DPP,&g_D3DPPFull,sizeof(D3DPRESENT_PARAMETERS));
		GetWindowRect(g_hWnd,&g_rcWnd);
	}

	HRESULT hr=g_pD3DDevice->Reset(&g_D3DPP);
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("ChangeDisplayMode Reset"),hr);
		if(hr==D3DERR_DEVICELOST) g_bDeviceLost=true;
		else DestroyWindow(g_hWnd);
		return hr;
	}

	hr=InitD3DObject();
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("ChangeDisplayMode InitD3DObject"),hr);
		DestroyWindow(g_hWnd);
		return hr;
	}

	if(g_bWndMode)		// ウィンドウモードに
	{
		SetWindowLong(g_hWnd,GWL_STYLE,WINDOW_STYLE);
		SetWindowPos(g_hWnd,0,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	else		// フルスクリーンモードに
	{
		DWORD w=g_D3DPPFull.BackBufferWidth, h=g_D3DPPFull.BackBufferHeight;
		SetWindowLong(g_hWnd,GWL_STYLE,FULLSCR_STYLE);
		SetWindowPos(g_hWnd,HWND_TOPMOST,0,0,w,h,SWP_DRAWFRAME);
	}

	return S_OK;
}

// DirectX Graphics 初期化
HRESULT InitDXGraphics(void)
{
	g_pD3D=Direct3DCreate9(D3D_SDK_VERSION);
	if(g_pD3D==NULL) return DXTRACE_ERR_MSGBOX(_T("InitDXGraphics Direct3DCreate9"),E_FAIL);

	UINT adapter;
	D3DDEVTYPE device;
	HRESULT hr=SelectD3DDevice(&adapter,&device);
	if(FAILED(hr)) DXTRACE_ERR_MSGBOX(_T("InitDxGraphics SelectD3DDevice"),hr);

	if(g_bWndMode) CopyMemory(&g_D3DPP,&g_D3DPPWnd,sizeof(D3DPRESENT_PARAMETERS));
	else CopyMemory(&g_D3DPP,&g_D3DPPFull,sizeof(D3DPRESENT_PARAMETERS));

	hr=g_pD3D->CreateDevice(adapter,device,g_hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,&g_D3DPP,&g_pD3DDevice);
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("InitDXGraphics CreateDevice D3DCREATE_HARDWARE_VERTEXPROCESSING"),hr);
		hr=g_pD3D->CreateDevice(adapter,device,g_hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,&g_D3DPP,&g_pD3DDevice);
		if(FAILED(hr))
		{
			return DXTRACE_ERR_MSGBOX(_T("InitDXGraphics CreateDevice D3DCREATE_SOFTWARE_VERTEXPROCESSING"),hr);
		}
	}

	D3DVIEWPORT9 vp;
	vp.X		= 0;
	vp.Y		= 0;
	vp.Width	= g_D3DPP.BackBufferWidth;
	vp.Height	= g_D3DPP.BackBufferHeight;
	vp.MinZ		= 0.0f;
	vp.MaxZ		= 1.0f;

	hr=g_pD3DDevice->SetViewport(&vp);
	if(FAILED(hr)) return DXTRACE_ERR_MSGBOX(_T("InitDXGraphics SetViewport"),hr);

	D3DXCreateSprite(g_pD3DDevice,&g_pSprite);

	TaskEx::InitTaskList();

	return S_OK;
}

// DirectX Graphics 解放
HRESULT CleanupDXGraphics(void)
{
	Task::ReleaseTaskList();

	SAFE_RELEASE(g_pD3DDevice);
	SAFE_RELEASE(g_pD3D);

	return S_OK;
}

// プロシージャ
LRESULT CALLBACK MainWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_F1: ChangeDisplayMode(); return 0;
		case VK_ESCAPE: PostMessage(hWnd,WM_CLOSE,0,0); return 0;
		}
		return 0;
	case WM_DISPLAYCHANGE:		// IDirect3DDevice9::Reset メソッドから Send される
		DisplayChange();
		return 0;
	case WM_SIZE:
		if(g_bWndMode==false || wParam==SIZE_MINIMIZED) return 0;
		g_D3DPP.BackBufferWidth	 = LOWORD(lParam);
		g_D3DPP.BackBufferHeight = HIWORD(lParam);
		if(wParam==SIZE_MAXIMIZED || wParam==SIZE_RESTORED) ChangeWindowSize();
		return 0;
	case WM_ACTIVATE:
		g_bActive=(LOWORD(wParam)!=WA_INACTIVE);
		return 0;
	case WM_DESTROY:
		CleanupD3DObject();
		CleanupDXGraphics();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd,msg,wParam,lParam);
}

// デバイス再作成
HRESULT ResetDevice(void)
{
	Sleep(100);

	HRESULT hr=g_pD3DDevice->TestCooperativeLevel();
	if(FAILED(hr))
	{
		if(hr==D3DERR_DEVICELOST)
		{
			g_bDeviceLost=true;
			return S_OK;
		}
		if(hr!=D3DERR_DEVICENOTRESET)
		{
			return DXTRACE_ERR_MSGBOX(_T("ResetDevice TestCooperativeLevel"),hr);
		}

		CleanupD3DObject();

		hr=g_pD3DDevice->Reset(&g_D3DPP);
		if(FAILED(hr))
		{
			if(hr==D3DERR_DEVICELOST)
			{
				g_bDeviceLost=true;
				return S_OK;
			}
			return DXTRACE_ERR_MSGBOX(_T("ResetDevice Reset"),hr);
		}

		hr=InitD3DObject();
		if(FAILED(hr))
		{
			return DXTRACE_ERR_MSGBOX(_T("ResetDevice InitD3DObject"),hr);
		}
	}
	g_bDeviceLost=false;

	return S_OK;
}

// メッセージが無い時の処理
bool AppIdle(void)
{
	if(!g_pD3D || !g_pD3DDevice)
	{
		DXTRACE_ERR_MSGBOX(_T("AppIdle g_pD3D==NULL || g_pD3DDevice==NULL"),E_FAIL);
		return false;
	}

	if(!g_bActive){ Sleep(1); return true; }

	if(g_bDeviceLost)
	{
		HRESULT hr=ResetDevice();
		if(FAILED(hr))
		{
			DXTRACE_ERR_MSGBOX(_T("AppIdle ResetDevice"),hr);
			return false;
		}
	}

	static DWORD beforeTime=timeGetTime();
	static DWORD frameCount=0,drawCount=0,sleepTime=0;

	DWORD nowTime=timeGetTime();
	DWORD progress=nowTime-beforeTime;

	frameCount++;
	DWORD time=(DWORD)(frameCount*1000.0/MAX_FPS+0.5);		// 理想の時間

	Task::RunTask();		// 値の更新

	if(progress<time)
	{
		nowTime=timeGetTime();
		progress=nowTime-beforeTime;

		if(progress<time)		// まだ時間に余裕がある
		{
			drawCount++;

			HRESULT hr=Render();		// 描画
			if(hr==D3DERR_DEVICELOST) g_bDeviceLost=true;
			else if(FAILED(hr))
			{
				DXTRACE_ERR_MSGBOX(_T("AppIdle Render"),hr);
				return false;
			}

			nowTime=timeGetTime();
			progress=nowTime-beforeTime;

			if(progress<time)		// まだ時間に余裕がある
			{
				Sleep(time-progress);
				sleepTime+=(time-progress);
				nowTime+=(time-progress);
				progress=time;
			}
		}
	}

	if(progress>=1000)
	{
		wsprintf(g_debugInfo,_T("FPS=%03d DRAW=%03d SLEEP=%03d"),frameCount,drawCount,sleepTime);
		beforeTime=nowTime;
		frameCount=0; drawCount=0; sleepTime=0;
	}

	return true;
}

// ウィンドウ作成
HRESULT InitApp(HINSTANCE hInst)
{
	g_hInst=hInst;

	WNDCLASS wc;
	wc.style		 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = MainWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInst;
	wc.hIcon		 = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName	 = NULL;
	wc.lpszClassName = _T(__FILE__);
	if(!RegisterClass(&wc)) return DXTRACE_ERR_MSGBOX(_T("InitApp RegisterClass"),GetLastError());

	RECT rc={0,0,WND_W,WND_H};
	AdjustWindowRect(&rc,WINDOW_STYLE,FALSE);

	g_hWnd=CreateWindow(
		_T(__FILE__),_T("本格的なシューティングゲームを実現するタスクシステム"),WINDOW_STYLE,
		CW_USEDEFAULT,CW_USEDEFAULT,rc.right-rc.left,rc.bottom-rc.top, 
		NULL,NULL,hInst,NULL);
	if(g_hWnd==NULL) return DXTRACE_ERR_MSGBOX(_T("InitApp CreateWindow"),GetLastError());

	return S_OK;
}

// メイン
int WINAPI _tWinMain(HINSTANCE hInst,HINSTANCE hPrevInst,LPSTR lpCmdLine,int nCmdShow)
{
	// ミューテックスオブジェクトによる多重起動の防止
	HANDLE hMutex=CreateMutex(NULL,TRUE,_T(__FILE__));
	if(GetLastError()==ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL,_T("既に起動しています"),_T("起動できません"),MB_OK);
		return 0;
	}

	HRESULT hr=InitApp(hInst);
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("_tWinMain InitApp"),hr);
		return 0;
	}

	hr=InitDXGraphics();
	if(FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("_tWinMain InitDXGraphics"),hr);
		DestroyWindow(g_hWnd);
	}
	else
	{
		hr=InitD3DObject();
		if(FAILED(hr))
		{
			DXTRACE_ERR_MSGBOX(_T("_tWinMain InitD3DObject"),hr);
			DestroyWindow(g_hWnd);
		}
	}

	if(g_bWndMode==false)		// フルスクリーンモードで起動
	{
		g_bWndMode=true;
		ChangeDisplayMode();
	}

	MSG msg;
	timeBeginPeriod(1);		// 最小分解能を1[ms]に設定する
	while(true)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			if(msg.message==WM_QUIT) break;
			DispatchMessage(&msg);
		}
		else
		{
			if(AppIdle()==false) DestroyWindow(g_hWnd);
		}
	}
	timeEndPeriod(1);

	CloseHandle(hMutex);

	return (int)msg.wParam;
}
