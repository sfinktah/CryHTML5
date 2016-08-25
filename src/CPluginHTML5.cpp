/* CryHTML5 - for licensing and copyright see license.txt */

#include "TwSimpleDX11.h"
#include <StdAfx.h>
#include <CPluginHTML5.h>
#include <CPluginD3D.h>


#include <CEFHandler.hpp>
#include <CEFCryPak.hpp>
#include <PMUtils.hpp>

#include <Cry_Camera.h>

// from cef osr demo
#include <Windowsx.h>
#include <cefclient/browser/geometry_util.h>
#include <cefclient/browser/main_message_loop.h>
#include <cefclient/browser/resource.h>
#include <cefclient/browser/util_win.h>


// Return the window's user data pointer.
template <typename T>
T GetUserDataPtr(HWND hWnd) {
	return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}


// end from cef osr demo

#pragma comment(lib, "libcef")
#pragma comment(lib, "libcef_dll_wrapper")

class CCamera;

namespace HTML5Plugin
{
    CPluginHTML5* gPlugin = NULL;
    D3DPlugin::IPluginD3D* gD3DSystem = NULL;
	// D3DPlugin::IPluginD3D * gPlugin = NULL;

	CPluginHTML5::CPluginHTML5() :
		m_refCEFHandler(nullptr),
		m_refCEFRequestContext(nullptr),
		m_refCEFFrame(nullptr)
	{
		gPlugin = this;
		gD3DSystem = nullptr;
	}

    CPluginHTML5::~CPluginHTML5()
    {
        Release( true );

        gPlugin = NULL;
    }

    bool CPluginHTML5::Release( bool bForce )
    {
        bool bRet = true;
        bool bWasInitialized = m_bIsFullyInitialized; // Will be reset by base class so backup

        if ( !m_bCanUnload )
        {
            // Note: Type Unregistration will be automatically done by the Base class (Through RegisterTypes)
            // Should be called while Game is still active otherwise there might be leaks/problems
            bRet = CPluginBase::Release( bForce );

            if ( bRet )
            {
                ShutdownDependencies();

                if ( bWasInitialized )
                {
                    // TODO: Cleanup stuff that can only be cleaned up if the plugin was initialized

                }

                // Cleanup like this always (since the class is static its cleaned up when the dll is unloaded)
                gPluginManager->UnloadPlugin( GetName() );

                // Allow Plugin Manager garbage collector to unload this plugin
                AllowDllUnload();
            }
        }

        return bRet;
    };

    bool CPluginHTML5::Init( SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory )
    // bool CPluginHTML5::Init()
    {
        bool bSuccess = true;

        // gPluginManager = ( PluginManager::IPluginManager* )pPluginManager->GetConcreteInterface( NULL );
        // bSuccess = CPluginBase::Init( env, startupParams, pPluginManager, sPluginDirectory );

        return bSuccess;
    }

    void Command_DevTools( IConsoleCmdArgs* pArgs )
    {
        gPlugin->ShowDevTools();
    };

    void Command_URL( IConsoleCmdArgs* pArgs )
    {
#ifdef WE_HAD_A_CONSOLE
        if ( pArgs->GetArgCount() > 1 )
        {
#undef GetCommandLine
            string sText( pArgs->GetCommandLine() );

            // read over the parameters
            size_t nOffset = sText.find_first_of( ' ' )   + 1;

            // remove parameters
            sText = sText.Mid( nOffset ).Trim();

            // delay the command
            if ( sText.length() > 0 )
            {
                gPlugin->SetURL( PluginManager::UTF82UCS2( sText ) );
            }
        }
#endif
    };

    void Command_JS( IConsoleCmdArgs* pArgs )
    {
#ifdef WE_HAD_A_CONSOLE
        if ( pArgs->GetArgCount() > 1 )
        {
#undef GetCommandLine
            string sText( pArgs->GetCommandLine() );

            // read over the parameters
            size_t nOffset = sText.find_first_of( ' ' )   + 1;

            // remove parameters
            sText = sText.Mid( nOffset ).Trim();

            // delay the command
            if ( sText.length() > 0 )
            {
                gPlugin->ExecuteJS( PluginManager::UTF82UCS2( sText ) );
            }
        }
#endif
    };

    void Command_Input( IConsoleCmdArgs* pArgs )
    {
#ifdef WE_HAD_A_CONSOLE
        if ( pArgs->GetArgCount() == 2 )
        {
            gPlugin->SetInputMode( PluginManager::ParseString<int>( pArgs->GetArg( 1 ) ) );
        }

        else if ( pArgs->GetArgCount() == 3 )
        {
            gPlugin->SetInputMode( PluginManager::ParseString<int>( pArgs->GetArg( 1 ) ), PluginManager::ParseString<bool>( pArgs->GetArg( 2 ) ) );
        }
#endif
    };

    bool CPluginHTML5::RegisterTypes( int nFactoryType, bool bUnregister )
    {
		bool bRet = TRUE; // or false, who knows
#ifdef WE_CARED
		// Note: Autoregister Flownodes will be automatically registered by the Base class
        bool bRet = CPluginBase::RegisterTypes( nFactoryType, bUnregister );

        using namespace PluginManager;
        eFactoryType enFactoryType = eFactoryType( nFactoryType );

        if ( bRet )
        {
            if ( gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
            {
                // CVars
                if ( gEnv->pConsole && ( enFactoryType == FT_All || enFactoryType == FT_CVar ) )
                {
                    if ( !bUnregister )
                    {
                        REGISTER_CVAR( cm5_active, 1.0f, VF_NULL, "CryHTML5 Rendering and systems active" );
                        REGISTER_CVAR( cm5_alphatest, 0.3f, VF_NULL, "CryHTML5 Alpha test threshold for cursor" );
                    }

                    else
                    {
                        gEnv->pConsole->UnregisterVariable( "cm5_active", true );
                        gEnv->pConsole->UnregisterVariable( "cm5_alphatest", true );
                    }
                }

                // CVars Commands
                if ( gEnv->pConsole && ( enFactoryType == FT_All || enFactoryType == FT_CVarCommand ) )
                {
                    if ( !bUnregister )
                    {
                        gEnv->pConsole->AddCommand( "cm5_devtools", Command_DevTools, VF_NULL, "Open the CryHTML5 DevTools" );
                        gEnv->pConsole->AddCommand( "cm5_url", Command_URL, VF_NULL, "Open the URL" );
                        gEnv->pConsole->AddCommand( "cm5_js", Command_JS, VF_NULL, "Execute the JavaScript" );
                        gEnv->pConsole->AddCommand( "cm5_input", Command_Input, VF_NULL, "Set Input mode" );
                    }

                    else
                    {
                        gEnv->pConsole->RemoveCommand( "cm5_devtools" );
                        gEnv->pConsole->RemoveCommand( "cm5_url" );
                        gEnv->pConsole->RemoveCommand( "cm5_js" );
                        gEnv->pConsole->RemoveCommand( "cm5_input" );
                    }
                }
            }
        }

#endif
        return bRet;
    }

    const char* CPluginHTML5::ListCVars() const
    {
        return "..."; // TODO: Enter CVARs/Commands here if you have some
    }

    const char* CPluginHTML5::GetStatus() const
    {
        return "OK";
    }

    bool CPluginHTML5::CheckDependencies()const
    {
        //Check For The Existence Of All Dependencies Here.
        bool bSuccess = CPluginBase::CheckDependencies();

        if ( bSuccess )
        {
            bSuccess = !!PluginManager::safeGetPluginConcreteInterface<D3DPlugin::IPluginD3D*>( "D3D" );
        }

        return bSuccess;
    }

    bool CPluginHTML5::InitDependencies()
    {
        //Initialize All Dependencies Here.
        bool bSuccess = true;

        bSuccess = InitD3DPlugin();

        HTML5Plugin::gPlugin->LogAlways( "InitD3DPlugin %s ", bSuccess ? "success" : "failed" );

        if ( bSuccess )
        {
            bSuccess = InitializeCEF();
        }

        if ( bSuccess )
        {
            bSuccess = CPluginBase::InitDependencies();
        }

        return bSuccess;
    }

    void CPluginHTML5::ShutdownDependencies()
    {
        // Shut Down All Dependencies Here.
        ShutdownD3DPlugin();

        // End
        ShutdownCEF();
    }

    bool CPluginHTML5::InitD3DPlugin()
    {
        //Tells This Instance To Depend On The D3D Plug-in.
        // gD3DSystem = PluginManager::safeUsePluginConcreteInterface<D3DPlugin::IPluginD3D*>( "D3D" );

		static D3DPlugin::CPluginD3D modulePlugin;
		D3DPlugin::gPlugin = &modulePlugin;
		D3DPlugin::gPlugin->InitWithoutPluginManager(); // modulePlugin.InitWithoutPluginManager();

		// IPluginD3DEx* m_pDXSystem;
		/*
		void* GetConcreteInterface( const char* sInterfaceVersion )
		{
			return static_cast <IPluginD3D*>( m_pDXSystem );
		};

		template<typename tCIFace>
		static tCIFace safeUsePluginConcreteInterface( const char* sPlugin, const char* sVersion = NULL )
		{
			IPluginBase* pBase = gPluginManager ? gPluginManager->GetPluginByName( sPlugin ) : NULL;
			tCIFace pPlugin = static_cast<tCIFace>( pBase ? pBase->GetConcreteInterface( sVersion ) : NULL );

			if ( pPlugin )
			{
				pBase->AddRef();
			}

			return pPlugin;
		};

		*/

        // gD3DSystem = PluginManager::safeUsePluginConcreteInterface<D3DPlugin::IPluginD3D*>( "D3D" );
		gD3DSystem = static_cast <D3DPlugin::IPluginD3D*>(modulePlugin.GetConcreteInterface(NULL));

		// When to init? And as what?
		// Apparently we must init before assigned gD3DSystem, else we will abort on the check beneath.

        //If We Could Not Resolve The D3D Dependency Then Return False.
        if ( !gD3DSystem )
        {
            gPlugin->LogError( "CPluginHTML5::InitD3DPlugin(): Failed To Get The D3D Plug-in (Plugin_D3D). This Plug-in (%s) Depends On The D3D Plug-in (Plugin_D3D).", GetName() );
            return false;
        }

        else
        {
			gD3DSystem->SetDevice(g_D3DDev);
			gD3DSystem->SetDeviceContext(g_D3DDevCtx);
			gD3DSystem->SetSwapChain(g_SwapChain);

            gD3DSystem->GetDevice(); // start search if isn't already found
        }

        return true;
    }

    void CPluginHTML5::ShutdownD3DPlugin()
    {
        if ( gD3DSystem )
        {
            // Do not release the listeners !!...

            //Un-Registers This Instance From The D3D Plug-in.
            PluginManager::safeReleasePlugin( "D3D", gD3DSystem );
        }
    }

    bool CPluginHTML5::InitializeCEF()
    {
        // Initialize Paths
        m_sCEFBrowserProcess = PluginManager::pathWithSeperator( gPluginManager->GetPluginDirectory( GetName() ) ) + "cefclient.exe";
        m_sCEFLog = PluginManager::pathWithSeperator( gPluginManager->GetDirectoryRoot() ) + "CryHTML5.log";
        m_sCEFResourceDir = gPluginManager->GetPluginDirectory( GetName() );
        m_sCEFLocalesDir = PluginManager::pathWithSeperator( gPluginManager->GetPluginDirectory( GetName() ) ) + "locales";

        // Initialize Settings
        CefSettings settings;
        CefString( &settings.browser_subprocess_path ).FromASCII( m_sCEFBrowserProcess.c_str() );
        CefString( &settings.log_file ).FromASCII( m_sCEFLog.c_str() );
        CefString( &settings.resources_dir_path ).FromASCII( m_sCEFResourceDir.c_str() );
        CefString( &settings.locales_dir_path ).FromASCII( m_sCEFLocalesDir.c_str() );

        settings.command_line_args_disabled = true;
        settings.multi_threaded_message_loop = true;
        settings.log_severity = LOGSEVERITY_WARNING;
        settings.remote_debugging_port = 8012;
        settings.background_color = CefColorSetARGB( 0, 0, 0, 0 );

        // Now initialize CEF
        CefMainArgs main_args;
        bool bSuccess = CefInitialize( main_args, settings, NULL, NULL );
        HTML5Plugin::gPlugin->LogAlways( "Initialize %s ", bSuccess ? "success" : "failed" );

        // Initialize Components
        if ( bSuccess )
        {
            CefRegisterSchemeHandlerFactory( "cry", "cry", new CEFCryPakHandlerFactory() );

            // Initialize a Browser
            bSuccess = InitializeCEFBrowser();
        }

        return bSuccess;
    }

    bool CPluginHTML5::InitializeCEFBrowser()
    {
        // Client Handler
        m_refCEFHandler = new CEFCryHandler( 1280, 720 );

        // Window Information
        CefWindowInfo info;

		info.transparent_painting_enabled = TRUE;
		info.windowless_rendering_enabled = TRUE;
		
        // info.SetAsOffScreen( HWND( gEnv->pRenderer->GetHWND() ) ); //info.SetAsOffScreen( NULL );
        // info.SetTransparentPainting( TRUE );

        // Register Listener for Render Handler
        gD3DSystem->RegisterListener( m_refCEFHandler->_renderHandler.get() );

        // Browser Settings
        CefBrowserSettings browserSettings;
        //browserSettings.accelerated_compositing = STATE_ENABLED; // For OSR always software is used this is an CEF restriction.
        //browserSettings.webgl = STATE_ENABLED;

        m_refCEFRequestContext = CefRequestContext::GetGlobalContext();

        bool bSuccess = CefBrowserHost::CreateBrowser( info, m_refCEFHandler.get(), "cry://UI/TestUI.html", browserSettings, nullptr );
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, m_refCEFHandler.get(), "http://www.youtube.com/watch?v=3MteSlpxCpo", browserSettings, nullptr );
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, m_refCEFHandler.get(), "http://webglsamples.googlecode.com/hg/aquarium/aquarium.html", browserSettings, nullptr );
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, g_handler.get(), "http://www.google.com", browserSettings, nullptr );

        HTML5Plugin::gPlugin->LogAlways( "CreateBrowser %s", bSuccess ? "success" : "failed" );
        return bSuccess;
    }

    void CPluginHTML5::ShowDevTools()
    {
        if ( !m_sCEFDebugURL.empty() )
        {
            LaunchExternalBrowser( m_sCEFDebugURL );
        }
    }

    void CPluginHTML5::LaunchExternalBrowser( const string& url )
    {
        if ( CefCurrentlyOn( TID_PROCESS_LAUNCHER ) )
        {
            // Retrieve the current executable path.
            CefString file_exe( gPlugin->m_sCEFBrowserProcess );

            // Create the command line.
            CefRefPtr<CefCommandLine> command_line =
                CefCommandLine::CreateCommandLine();
            command_line->SetProgram( file_exe );
            command_line->AppendSwitchWithValue( "url", url.c_str() );

            // Launch the process.
            CefLaunchProcess( command_line );
        }

        else
        {
            // Execute on the PROCESS_LAUNCHER thread.
            CefPostTask( TID_PROCESS_LAUNCHER, NewCefRunnableFunction( &CPluginHTML5::LaunchExternalBrowser, url ) );
        }
    }

    void CPluginHTML5::ShutdownCEF()
    {
        gPlugin->LogAlways( "Shutting down" );

        // m_refCEFHandler->m_input.UnregisterListeners();

        m_refCEFFrame = nullptr;
        m_refCEFHandler = nullptr;
        m_refCEFRequestContext = nullptr;

        CefShutdown();

        gPlugin->LogAlways( "Closed" );
    }

    bool CPluginHTML5::SetURL( const wchar_t* sURL )
    {
        if ( m_refCEFFrame.get() != nullptr )
        {
            m_refCEFFrame->LoadURL( sURL );
            return true;
        }

        return false;
    }

    bool CPluginHTML5::ExecuteJS( const wchar_t* sJS )
    {
        if ( m_refCEFFrame.get() != nullptr )
        {
            m_refCEFFrame->ExecuteJavaScript( sJS, CefString( "CryHTML" ), 0 );
            return true;
        }

        return false;
    }

#ifndef __FUCK_U
    bool CPluginHTML5::WorldPosToScreenPos( CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset /*= Vec3( ZERO ) */ )
    {
        if ( m_refCEFHandler.get() != nullptr && m_refCEFHandler->_renderHandler.get() != nullptr )
        {
            // get current camera matrix
            const Matrix34& camMat = cam.GetMatrix();

            // add offset to position
            const Vec3 vFaceingPos = camMat.GetTranslation() - camMat.GetColumn1() * 1000.f;
            const Vec3 vDir = ( vWorld - vFaceingPos ).GetNormalizedFast();
            const Vec3 vOffsetX = vDir.Cross( Vec3Constants<float>::fVec3_OneZ ).GetNormalizedFast() * vOffset.x;
            const Vec3 vOffsetY = vDir * vOffset.y;
            const Vec3 vOffsetZ = Vec3( 0, 0, vOffset.z );
            vWorld += vOffsetX + vOffsetY + vOffsetZ;

            // calculate screen x,y coordinates
            cam.SetMatrix( camMat );
            cam.Project( vWorld, vScreen );
            ScaleCoordinates( vScreen.x, vScreen.y, vScreen.x, vScreen.y, false, true );

            // store depth in z
            vScreen.z = ( camMat.GetTranslation() - vWorld ).GetLength();

            return true;
        }

        return false;
    }
#endif

    void CPluginHTML5::ScaleCoordinates( float fX, float fY, float& foX, float& foY, bool bLimit /*= false*/, bool bCERenderer /*= true */ )
    {
        if ( m_refCEFHandler.get() != nullptr )
        {
            m_refCEFHandler->_renderHandler->ScaleCoordinates( fX, fY, foX, foY, bLimit, bCERenderer );
        }
    }

    void CPluginHTML5::SetInputMode( int nMode, bool bExclusive )
    {
        if ( m_refCEFHandler.get() != nullptr )
        {
            // m_refCEFHandler->m_input.SetInputMode( nMode, bExclusive );
        }
    }

    void CPluginHTML5::SetActive( bool bActive )
    {
        cm5_active = bActive ? 1.0f : 0.0f;
    }

    bool CPluginHTML5::IsCursorOnSurface()
    {
        if ( cm5_active > 0.0 && m_refCEFHandler.get() != nullptr )
        {
            float fX = 0.0f, fY = 0.0f;
            // m_refCEFHandler->m_input.GetCursorPos( fX, fY );

            return IsOpaque( fX, fY );
        }

        return false;
    }

    bool CPluginHTML5::IsOpaque( float fX, float fY )
    {
		
		/*
        if ( cm5_active > 0.0 && m_refCEFHandler.get() != nullptr )
        {
            const ColorB& color = m_refCEFHandler->_renderHandler->GetPixel( fX, fY );

            return color.a >= ( cm5_alphatest * 255 );
        }
		*/

        return false;
    }

	// This is copied from CEF offscren demo cefclient_osr_widget_win section
	bool CPluginHTML5::isKeyDown(WPARAM wparam) {
		return (GetKeyState(wparam) & 0x8000) != 0;
	}

	int CPluginHTML5::GetCefMouseModifiers(WPARAM wparam) {
		int modifiers = 0;
		if (wparam & MK_CONTROL)
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		if (wparam & MK_SHIFT)
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		if (isKeyDown(VK_MENU))
			modifiers |= EVENTFLAG_ALT_DOWN;
		if (wparam & MK_LBUTTON)
			modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
		if (wparam & MK_MBUTTON)
			modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		if (wparam & MK_RBUTTON)
			modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

		// Low bit set from GetKeyState indicates "toggled".
		if (::GetKeyState(VK_NUMLOCK) & 1)
			modifiers |= EVENTFLAG_NUM_LOCK_ON;
		if (::GetKeyState(VK_CAPITAL) & 1)
			modifiers |= EVENTFLAG_CAPS_LOCK_ON;
		return modifiers;
	}

	int CPluginHTML5::GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
		int modifiers = 0;
		if (isKeyDown(VK_SHIFT))
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		if (isKeyDown(VK_CONTROL))
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		if (isKeyDown(VK_MENU))
			modifiers |= EVENTFLAG_ALT_DOWN;

		// Low bit set from GetKeyState indicates "toggled".
		if (::GetKeyState(VK_NUMLOCK) & 1)
			modifiers |= EVENTFLAG_NUM_LOCK_ON;
		if (::GetKeyState(VK_CAPITAL) & 1)
			modifiers |= EVENTFLAG_CAPS_LOCK_ON;

		switch (wparam) {
		case VK_RETURN:
			if ((lparam >> 16) & KF_EXTENDED)
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if (!((lparam >> 16) & KF_EXTENDED))
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_NUMLOCK:
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
		case VK_DIVIDE:
		case VK_MULTIPLY:
		case VK_SUBTRACT:
		case VK_ADD:
		case VK_DECIMAL:
		case VK_CLEAR:
			modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_SHIFT:
			if (isKeyDown(VK_LSHIFT))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RSHIFT))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_CONTROL:
			if (isKeyDown(VK_LCONTROL))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RCONTROL))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_MENU:
			if (isKeyDown(VK_LMENU))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RMENU))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_LWIN:
			modifiers |= EVENTFLAG_IS_LEFT;
			break;
		case VK_RWIN:
			modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		}
		return modifiers;
	}

	//bool CPluginHTML5::IsOverPopupWidget(int x, int y) const {
// 	//	const CefRect& rc = renderer_.popup_rect();
	//	int popup_right = rc.x + rc.width;
	//	int popup_bottom = rc.y + rc.height;
	//	return (x >= rc.x) && (x < popup_right) &&
	//		(y >= rc.y) && (y < popup_bottom);
	//}

	//int CPluginHTML5::GetPopupXOffset() const {
// 	//	return renderer_.original_popup_rect().x - renderer_.popup_rect().x;
	//}

	//int CPluginHTML5::GetPopupYOffset() const {
// 	//	return renderer_.original_popup_rect().y - renderer_.popup_rect().y;
	//}

	//void CPluginHTML5::ApplyPopupOffset(int& x, int& y) const {
	//	if (IsOverPopupWidget(x, y)) {
	//		x += GetPopupXOffset();
	//		y += GetPopupYOffset();
	//	}
	//}


	// end copy from section

	// This is copied from CEF offscreen demo, uncomment or comment as needed

	//void CPluginHTML5::CreateBrowser(HWND parent_hwnd,
	//	const RECT& rect,
	//	CefRefPtr<CefClient> handler,
	//	const CefBrowserSettings& settings,
	//	CefRefPtr<CefRequestContext> request_context,
	//	const std::string& startup_url) {
	//	if (!CefCurrentlyOn(TID_UI)) {
	//		// Execute this method on the UI thread.
	//		CefPostTask(TID_UI, base::Bind(&CPluginHTML5::CreateBrowser, this,
	//			parent_hwnd, rect, handler, settings,
	//			request_context, startup_url));
	//		return;
	//	}

	//	// Create the native window.
	//	Create(parent_hwnd, rect);

	//	CefWindowInfo window_info;
// 	//	window_info.SetAsWindowless(hwnd_, renderer_.IsTransparent());
	//	// settings.background_color = 0xffffff80;
	//	// Create the browser asynchronously.
	//	CefBrowserHost::CreateBrowser(window_info, handler, startup_url, settings,
	//		request_context);
	//}

	void CPluginHTML5::ShowPopup(HWND parent_hwnd,
		int x, int y, size_t width, size_t height) {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::ShowPopup, this,
				parent_hwnd, x, y, width, height));
			return;
		}

		DCHECK(browser_.get());

		// Create the native window.
		const RECT rect = {x, y,
			x + static_cast<int>(width),
			y + static_cast<int>(height)};
		//Create(parent_hwnd, rect);

		// Send resize notification so the compositor is assigned the correct
		// viewport size and begins rendering.
		browser_->GetHost()->WasResized();

		Show();
	}

	void CPluginHTML5::Show() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::Show, this));
			return;
		}

		if (!browser_)
			return;

		// Show the native window if not currently visible.
		if (hwnd_ && !::IsWindowVisible(hwnd_))
			ShowWindow(hwnd_, SW_SHOW);

		if (hidden_) {
			// Set the browser as visible.
			browser_->GetHost()->WasHidden(false);
			hidden_ = false;
		}

		// Give focus to the browser.
		browser_->GetHost()->SendFocusEvent(true);
	}

	void CPluginHTML5::Hide() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::Hide, this));
			return;
		}

		if (!browser_)
			return;

		// Remove focus from the browser.
		browser_->GetHost()->SendFocusEvent(false);

		if (!hidden_) {
			// Set the browser as hidden.
			browser_->GetHost()->WasHidden(true);
			hidden_ = true;
		}
	}

	void CPluginHTML5::SetBounds(int x, int y, size_t width, size_t height) {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::SetBounds, this, x, y, width,
				height));
			return;
		}

		if (hwnd_) {
			// Set the browser window bounds.
			::SetWindowPos(hwnd_, NULL, x, y,
				static_cast<int>(width), static_cast<int>(height),
				SWP_NOZORDER);
		}
	}

	void CPluginHTML5::SetFocus() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::SetFocus, this));
			return;
		}

		if (hwnd_) {
			// Give focus to the native window.
			::SetFocus(hwnd_);
		}
	}

	void CPluginHTML5::SetDeviceScaleFactor(float device_scale_factor_width, float device_scale_factor_height) {
		//if (!CefCurrentlyOn(TID_UI)) {
		//	// Execute this method on the UI thread.
		//	CefPostTask(TID_UI, base::Bind(&CPluginHTML5::SetDeviceScaleFactor, this,
		//		device_scale_factor));
		//	return;
		//}

		if (device_scale_factor_width_  == device_scale_factor_width
		 && device_scale_factor_height_ == device_scale_factor_height)
			return;

		device_scale_factor_width_ = device_scale_factor_width;
		device_scale_factor_height_ = device_scale_factor_height;
		if (browser_) {
			browser_->GetHost()->NotifyScreenInfoChanged();
			browser_->GetHost()->WasResized();
		}
		// device_scale_factor_ = device_scale_factor;
	}

//	void CPluginHTML5::Create(HWND parent_hwnd, const RECT& rect) {
//		CEF_REQUIRE_UI_THREAD();
//		DCHECK(!hwnd_ && !hdc_ && !hrc_);
//		DCHECK(parent_hwnd);
//		DCHECK(!::IsRectEmpty(&rect));
//
//		HINSTANCE hInst = ::GetModuleHandle(NULL);
//
// //		const cef_color_t background_color = renderer_.GetBackgroundColor();
//		const HBRUSH background_brush = CreateSolidBrush(
//			RGB(CefColorGetR(background_color),
//				CefColorGetG(background_color),
//				CefColorGetB(background_color)));
//
//		RegisterOsrClass(hInst, background_brush);
//
//		// Create the native window with a border so it's easier to visually identify
//		// OSR windows.
//		hwnd_ = ::CreateWindow(kWndClass, 0,
//			WS_BORDER | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
//			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
//			parent_hwnd, 0, hInst, 0);
//		CHECK(hwnd_);
//
//		client_rect_ = rect;
//
//		// Associate |this| with the window.
//		SetUserDataPtr(hwnd_, this);
//
//#if defined(CEF_USE_ATL)
//		// Create/register the drag&drop handler.
//		drop_target_ = DropTargetWin::Create(this, hwnd_);
//		HRESULT register_res = RegisterDragDrop(hwnd_, drop_target_);
//		DCHECK_EQ(register_res, S_OK);
//#endif
//
//		// Notify the window owner.
//		NotifyNativeWindowCreated(hwnd_);
//	}


//	void CPluginHTML5::Destroy() {
//		CEF_REQUIRE_UI_THREAD();
//		DCHECK(hwnd_ != NULL);
//
//#if defined(CEF_USE_ATL)
//		// Revoke/delete the drag&drop handler.
//		RevokeDragDrop(hwnd_);
//		drop_target_ = NULL;
//#endif
//
//		DisableGL();
//
//		// Destroy the native window.
//		::DestroyWindow(hwnd_);
//		hwnd_ = NULL;
//	}

	//void CPluginHTML5::EnableGL() {
	//	CEF_REQUIRE_UI_THREAD();

	//	PIXELFORMATDESCRIPTOR pfd;
	//	int format;

	//	// Get the device context.
	//	hdc_ = GetDC(hwnd_);

	//	// Set the pixel format for the DC.
	//	ZeroMemory(&pfd, sizeof(pfd));
	//	pfd.nSize = sizeof(pfd);
	//	pfd.nVersion = 1;
	//	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	//	pfd.iPixelType = PFD_TYPE_RGBA;
	//	pfd.cColorBits = 24;
	//	pfd.cDepthBits = 16;
	//	pfd.iLayerType = PFD_MAIN_PLANE;
	//	format = ChoosePixelFormat(hdc_, &pfd);
	//	SetPixelFormat(hdc_, format, &pfd);

	//	// Create and enable the render context.
	//	hrc_ = wglCreateContext(hdc_);

	//	ScopedGLContext scoped_gl_context(hdc_, hrc_, false);
// 	//	renderer_.Initialize();
	//}

	//void CPluginHTML5::DisableGL() {
	//	CEF_REQUIRE_UI_THREAD();

	//	if (!hdc_)
	//		return;

	//	{
	//		ScopedGLContext scoped_gl_context(hdc_, hrc_, false);
// 	//		renderer_.Cleanup();
	//	}

	//	if (IsWindow(hwnd_)) {
	//		// wglDeleteContext will make the context not current before deleting it.
	//		BOOL result = wglDeleteContext(hrc_);
	//		ALLOW_UNUSED_LOCAL(result);
	//		DCHECK(result);
	//		ReleaseDC(hwnd_, hdc_);
	//	}

	//	hdc_ = NULL;
	//	hrc_ = NULL;
	//}

	//void CPluginHTML5::Invalidate() {
	//	CEF_REQUIRE_UI_THREAD();

	//	// Don't post another task if the previous task is still pending.
	//	if (render_task_pending_)
	//		return;
	//	render_task_pending_ = true;

	//	CefPostDelayedTask(TID_UI, base::Bind(&CPluginHTML5::Render, this),
	//		kRenderDelay);
	//}

	//void CPluginHTML5::Render() {
	//	CEF_REQUIRE_UI_THREAD();

	//	if (render_task_pending_)
	//		render_task_pending_ = false;

	//	if (!hdc_)
	//		EnableGL();

	//	ScopedGLContext scoped_gl_context(hdc_, hrc_, true);
// 	//	renderer_.Render();
	//}

	//void CPluginHTML5::NotifyNativeWindowCreated(HWND hwnd) {
	//	if (!CURRENTLY_ON_MAIN_THREAD()) {
	//		// Execute this method on the main thread.
	//		MAIN_POST_CLOSURE(
	//			base::Bind(&CPluginHTML5::NotifyNativeWindowCreated, this, hwnd));
	//		return;
	//	}

	//	delegate_->OnOsrNativeWindowCreated(hwnd);
	//}

	// static
	//void CPluginHTML5::RegisterOsrClass(HINSTANCE hInstance,
	//	HBRUSH background_brush) {
	//	// Only register the class one time.
	//	static bool class_registered = false;
	//	if (class_registered)
	//		return;
	//	class_registered = true;

	//	WNDCLASSEX wcex;

	//	wcex.cbSize = sizeof(WNDCLASSEX);
	//	wcex.style         = CS_OWNDC;
	//	wcex.lpfnWndProc   = OsrWndProc;
	//	wcex.cbClsExtra    = 0;
	//	wcex.cbWndExtra    = 0;
	//	wcex.hInstance     = hInstance;
	//	wcex.hIcon         = NULL;
	//	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	//	wcex.hbrBackground = background_brush;
	//	wcex.lpszMenuName  = NULL;
	//	wcex.lpszClassName = kWndClass;
	//	wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	//	RegisterClassEx(&wcex);
	//}

	// static
	LRESULT CALLBACK CPluginHTML5::OsrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		//CEF_REQUIRE_UI_THREAD();
		//gPlugin->LogError("CPluginHTML5::OsrWndProc: CefCurrentlyOn() = %i", CefCurrentlyOn(TID_UI));

		// CPluginHTML5* self = GetUserDataPtr<CPluginHTML5*>(hWnd);
		CPluginHTML5* self = HTML5Plugin::gPlugin;
		if (!self)
			return DefWindowProc(hWnd, message, wParam, lParam);

		switch (message) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_MOUSELEAVE:
		case WM_MOUSEWHEEL:
			self->OnMouseEvent(message, wParam, lParam);
			break;

		case WM_SIZING:
			self->OnSizing(hWnd, message, wParam, lParam);
			break;
		case WM_SIZE:
			self->OnSize();
			break;

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			self->OnFocus(message == WM_SETFOCUS);
			break;

		case WM_CAPTURECHANGED:
		case WM_CANCELMODE:
			self->OnCaptureLost();
			break;

		case WM_SYSCHAR:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
			self->OnKeyEvent(message, wParam, lParam);
			break;

		//case WM_PAINT:
		//	self->OnPaint();
		//	return 0;

		//case WM_ERASEBKGND:
		//	if (self->OnEraseBkgnd())
		//		break;
		//	// Don't erase the background.
		//	return 0;

		//case WM_NCDESTROY:
		//	// Clear the reference to |self|.
		//	SetUserDataPtr(hWnd, NULL);
		//	self->hwnd_ = NULL;
		//	break;
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	void CPluginHTML5::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam) {
		CefRefPtr<CefBrowserHost> browser_host;
		if (browser_)
			browser_host = browser_->GetHost();

		LONG currentTime = 0;
		bool cancelPreviousClick = false;

		if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
			message == WM_MBUTTONDOWN || message == WM_MOUSEMOVE ||
			message == WM_MOUSELEAVE) {
			currentTime = GetMessageTime();
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			cancelPreviousClick =
				(abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2))
				|| (abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2))
				|| ((currentTime - last_click_time_) > GetDoubleClickTime());
			if (cancelPreviousClick &&
				(message == WM_MOUSEMOVE || message == WM_MOUSELEAVE)) {
				last_click_count_ = 0;
				last_click_x_ = 0;
				last_click_y_ = 0;
				last_click_time_ = 0;
			}
		}

		switch(message) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN: {
			::SetCapture(hwnd_);
			::SetFocus(hwnd_);
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			if (wParam & MK_SHIFT) {
				// Start rotation effect.
				last_mouse_pos_.x = current_mouse_pos_.x = x;
				last_mouse_pos_.y = current_mouse_pos_.y = y;
				mouse_rotation_ = true;
			} else {
				CefBrowserHost::MouseButtonType btnType =
					(message == WM_LBUTTONDOWN ? MBT_LEFT : (
						message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
				if (!cancelPreviousClick && (btnType == last_click_button_)) {
					++last_click_count_;
				} else {
					last_click_count_ = 1;
					last_click_x_ = x;
					last_click_y_ = y;
				}
				last_click_time_ = currentTime;
				last_click_button_ = btnType;

				if (browser_host) {
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					//last_mouse_down_on_view_ = !IsOverPopupWidget(x, y);
					//ApplyPopupOffset(mouse_event.x, mouse_event.y);
					client::DeviceToLogical(mouse_event, device_scale_factor_width_, device_scale_factor_height_);
					mouse_event.modifiers = GetCefMouseModifiers(wParam);
					browser_host->SendMouseClickEvent(mouse_event, btnType, false,
						last_click_count_);
				}
			}
		} break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			if (GetCapture() == hwnd_)
				ReleaseCapture();
			if (mouse_rotation_) {
				// End rotation effect.
				mouse_rotation_ = false;
// 				//renderer_.SetSpin(0, 0);
				//Invalidate();
			} else {
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);
				CefBrowserHost::MouseButtonType btnType =
					(message == WM_LBUTTONUP ? MBT_LEFT : (
						message == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));
				if (browser_host) {
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					//if (last_mouse_down_on_view_ &&
					//	IsOverPopupWidget(x, y) &&
					//	(GetPopupXOffset() || GetPopupYOffset())) {
					//	break;
					//}
					//ApplyPopupOffset(mouse_event.x, mouse_event.y);
					client::DeviceToLogical(mouse_event, device_scale_factor_width_, device_scale_factor_height_);
					mouse_event.modifiers = GetCefMouseModifiers(wParam);
					browser_host->SendMouseClickEvent(mouse_event, btnType, true,
						last_click_count_);
				}
			}
			break;

		case WM_MOUSEMOVE: {
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			if (mouse_rotation_) {
				// Apply rotation effect.
				current_mouse_pos_.x = x;
				current_mouse_pos_.y = y;
// 				renderer_.IncrementSpin(
					//current_mouse_pos_.x - last_mouse_pos_.x,
					//current_mouse_pos_.y - last_mouse_pos_.y);
				last_mouse_pos_.x = current_mouse_pos_.x;
				last_mouse_pos_.y = current_mouse_pos_.y;
				//Invalidate();
			} else {
				if (!mouse_tracking_) {
					// Start tracking mouse leave. Required for the WM_MOUSELEAVE event to
					// be generated.
					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hwnd_;
					TrackMouseEvent(&tme);
					mouse_tracking_ = true;
				}

				if (browser_host) {
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					//ApplyPopupOffset(mouse_event.x, mouse_event.y);
					client::DeviceToLogical(mouse_event, device_scale_factor_width_, device_scale_factor_height_);
					mouse_event.modifiers = GetCefMouseModifiers(wParam);
					browser_host->SendMouseMoveEvent(mouse_event, false);
				}
			}
			break;
		}

		case WM_MOUSELEAVE: {
			if (mouse_tracking_) {
				// Stop tracking mouse leave.
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE & TME_CANCEL;
				tme.hwndTrack = hwnd_;
				TrackMouseEvent(&tme);
				mouse_tracking_ = false;
			}

			if (browser_host) {
				// Determine the cursor position in screen coordinates.
				POINT p;
				::GetCursorPos(&p);
				::ScreenToClient(hwnd_, &p);

				CefMouseEvent mouse_event;
				mouse_event.x = p.x;
				mouse_event.y = p.y;
				client::DeviceToLogical(mouse_event, device_scale_factor_width_, device_scale_factor_height_);
				mouse_event.modifiers = GetCefMouseModifiers(wParam);
				browser_host->SendMouseMoveEvent(mouse_event, true);
			}
		} break;

		case WM_MOUSEWHEEL:
			if (browser_host) {
				POINT screen_point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				HWND scrolled_wnd = ::WindowFromPoint(screen_point);
				if (scrolled_wnd != hwnd_)
					break;

				ScreenToClient(hwnd_, &screen_point);
				int delta = GET_WHEEL_DELTA_WPARAM(wParam);

				CefMouseEvent mouse_event;
				mouse_event.x = screen_point.x;
				mouse_event.y = screen_point.y;
				//ApplyPopupOffset(mouse_event.x, mouse_event.y);
				client::DeviceToLogical(mouse_event, device_scale_factor_width_, device_scale_factor_height_);
				mouse_event.modifiers = GetCefMouseModifiers(wParam);
				browser_host->SendMouseWheelEvent(mouse_event,
					client::IsKeyDown(VK_SHIFT) ? delta : 0,
					!client::IsKeyDown(VK_SHIFT) ? delta : 0);
			}
			break;
		}
	}


	void CPluginHTML5::OnSizing(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
		LPRECT rect = (RECT *)(lParam);
		LONG width;
		LONG widthIdeal;
		LONG height;
		FLOAT ratio;
		hwnd_ = hwnd;
		width = rect->right - rect->left;
		height = rect->bottom - rect->top;
		ratio = 1280.0f / 720.0f;
		widthIdeal = height * ratio;
		// rect->right = rect->left + widthIdeal;
 		// SetDeviceScaleFactor(widthIdeal / 1280.0f);
		::GetClientRect(hwnd_, &client_rect_);
		SetDeviceScaleFactor(
			(client_rect_.right - client_rect_.left) / 1280.0f,
			(client_rect_.bottom - client_rect_.top) / 720.0f
		);
	}

	//void CPluginHTML5::OnSize() {
	//	// Keep |client_rect_| up to date.
	//	::GetClientRect(hwnd_, &client_rect_);

	//	SetDeviceScaleFactor(
	//		(client_rect_.right - client_rect_.left) / 1280.0f,
	//		(client_rect_.bottom - client_rect_.top) / 720.0f
	//	);
	//	if (browser_)
	//		browser_->GetHost()->WasResized();
	//}

	void CPluginHTML5::OnSize() {
		// Keep |client_rect_| up to date.
		::GetClientRect(hwnd_, &client_rect_);

		if (browser_)
			browser_->GetHost()->WasResized();
	}



	void CPluginHTML5::OnFocus(bool setFocus) {
		if (browser_)
			browser_->GetHost()->SendFocusEvent(setFocus);
	}

	void CPluginHTML5::OnCaptureLost() {
		if (mouse_rotation_)
			return;

		if (browser_)
			browser_->GetHost()->SendCaptureLostEvent();
	}

	void CPluginHTML5::OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {
		if (!browser_)
			return;

		CefKeyEvent event;
		event.windows_key_code = wParam;
		event.native_key_code = lParam;
		event.is_system_key = 
			message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;

		if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
			event.type = KEYEVENT_RAWKEYDOWN;
		else if (message == WM_KEYUP || message == WM_SYSKEYUP)
			event.type = KEYEVENT_KEYUP;
		else
			event.type = KEYEVENT_CHAR;
		event.modifiers = GetCefKeyboardModifiers(wParam, lParam);

		browser_->GetHost()->SendKeyEvent(event);
	}

	void CPluginHTML5::OnPaint() {
		//// Paint nothing here. Invalidate will cause OnPaint to be called for the
		//// render handler.
		//PAINTSTRUCT ps;
		//BeginPaint(hwnd_, &ps);
		//EndPaint(hwnd_, &ps);

		//if (browser_)
		//	browser_->GetHost()->invalidate(PET_VIEW);
	}

	bool CPluginHTML5::OnEraseBkgnd() {
		// Erase the background when the browser does not exist.
		return (browser_ == NULL);
	}

//	bool CPluginHTML5::IsOverPopupWidget(int x, int y) const {
//		CEF_REQUIRE_UI_THREAD();
//// 		const CefRect& rc = renderer_.popup_rect();
//		int popup_right = rc.x + rc.width;
//		int popup_bottom = rc.y + rc.height;
//		return (x >= rc.x) && (x < popup_right) &&
//			(y >= rc.y) && (y < popup_bottom);
//	}
//
//	int CPluginHTML5::GetPopupXOffset() const {
//		CEF_REQUIRE_UI_THREAD();
//// 		return renderer_.original_popup_rect().x - renderer_.popup_rect().x;
//	}
//
//	int CPluginHTML5::GetPopupYOffset() const {
//		CEF_REQUIRE_UI_THREAD();
//// 		return renderer_.original_popup_rect().y - renderer_.popup_rect().y;
//	}
//
//	void CPluginHTML5::ApplyPopupOffset(int& x, int& y) const {
//		if (IsOverPopupWidget(x, y)) {
//			x += GetPopupXOffset();
//			y += GetPopupYOffset();
//		}
//	}

	void CPluginHTML5::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
		CEF_REQUIRE_UI_THREAD();
		DCHECK(!browser_);
		browser_ = browser;

		if (hwnd_) {
			// Show the browser window. Called asynchronously so that the browser has
			// time to create associated internal objects.
			CefPostTask(TID_UI, base::Bind(&CPluginHTML5::Show, this));
		}
	}

	//void CPluginHTML5::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
	//	CEF_REQUIRE_UI_THREAD();
	//	// Detach |this| from the ClientHandlerOsr.
	//	static_cast<ClientHandlerOsr*>(browser_->GetHost()->GetClient().get())->
	//		DetachOsrDelegate();
	//	browser_ = NULL;
	//	Destroy();
	//}

	bool CPluginHTML5::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
		CefRect& rect) {
		CEF_REQUIRE_UI_THREAD();
		return false;
	}

	bool CPluginHTML5::GetViewRect(CefRefPtr<CefBrowser> browser,
		CefRect& rect) {
		CEF_REQUIRE_UI_THREAD();

		rect.x = rect.y = 0;
		rect.width = client::DeviceToLogical(client_rect_.right - client_rect_.left,
			device_scale_factor_width_);
		rect.height = client::DeviceToLogical(client_rect_.bottom - client_rect_.top,
			device_scale_factor_height_);
		return true;
	}

	bool CPluginHTML5::GetScreenPoint(CefRefPtr<CefBrowser> browser,
		int viewX,
		int viewY,
		int& screenX,
		int& screenY) {
		CEF_REQUIRE_UI_THREAD();

		if (!::IsWindow(hwnd_))
			return false;

		// Convert the point from view coordinates to actual screen coordinates.
		POINT screen_pt = {
			client::LogicalToDevice(viewX, device_scale_factor_width_),
			client::LogicalToDevice(viewY, device_scale_factor_height_)
		};
		ClientToScreen(hwnd_, &screen_pt);
		screenX = screen_pt.x;
		screenY = screen_pt.y;
		return true;
	}

	bool CPluginHTML5::GetScreenInfo(CefRefPtr<CefBrowser> browser,
		CefScreenInfo& screen_info) {
		CEF_REQUIRE_UI_THREAD();

		if (!::IsWindow(hwnd_))
			return false;

		CefRect view_rect;
		GetViewRect(browser, view_rect);

		screen_info.device_scale_factor = device_scale_factor_width_;

		// The screen info rectangles are used by the renderer to create and position
		// popups. Keep popups inside the view rectangle.
		screen_info.rect = view_rect;
		screen_info.available_rect = view_rect;
		return true;
	}

//	void CPluginHTML5::OnPopupShow(CefRefPtr<CefBrowser> browser,
//		bool show) {
//		CEF_REQUIRE_UI_THREAD();
//
//		if (!show) {
//// 			renderer_.ClearPopupRects();
//			browser->GetHost()->Invalidate(PET_VIEW);
//		}
//// 		renderer_.OnPopupShow(browser, show);
//	}

//	void CPluginHTML5::OnPopupSize(CefRefPtr<CefBrowser> browser,
//		const CefRect& rect) {
//		CEF_REQUIRE_UI_THREAD();
//
//// 		renderer_.OnPopupSize(browser, client::LogicalToDevice(rect, device_scale_factor_));
//	}

	//void CPluginHTML5::OnPaint(CefRefPtr<CefBrowser> browser,
	//	CefRenderHandler::PaintElementType type,
	//	const CefRenderHandler::RectList& dirtyRects,
	//	const void* buffer,
	//	int width,
	//	int height) {
	//	CEF_REQUIRE_UI_THREAD();

	//	if (painting_popup_) {
// 	//		renderer_.OnPaint(browser, type, dirtyRects, buffer, width, height);
	//		return;
	//	}
	//	if (!hdc_)
	//		EnableGL();

	//	ScopedGLContext scoped_gl_context(hdc_, hrc_, true);
// 	//	renderer_.OnPaint(browser, type, dirtyRects, buffer, width, height);
// 	//	if (type == PET_VIEW && !renderer_.popup_rect().IsEmpty()) {
	//		painting_popup_ = true;
	//		browser->GetHost()->Invalidate(PET_POPUP);
	//		painting_popup_ = false;
	//	}
// 	//	renderer_.Render();
	//}

	void CPluginHTML5::OnCursorChange(
		CefRefPtr<CefBrowser> browser,
		CefCursorHandle cursor,
		CefRenderHandler::CursorType type,
		const CefCursorInfo& custom_cursor_info) {
		CEF_REQUIRE_UI_THREAD();

		if (!::IsWindow(hwnd_))
			return;

		// Change the plugin window's cursor.
		SetClassLongPtr(hwnd_, GCLP_HCURSOR,
			static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
		SetCursor(cursor);
	}

	bool CPluginHTML5::StartDragging(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefDragData> drag_data,
		CefRenderHandler::DragOperationsMask allowed_ops,
		int x, int y) {
		CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
		if (!drop_target_)
			return false;

		current_drag_op_ = DRAG_OPERATION_NONE;
		CefBrowserHost::DragOperationsMask result =
			drop_target_->StartDragging(browser, drag_data, allowed_ops, x, y);
		current_drag_op_ = DRAG_OPERATION_NONE;
		POINT pt = {};
		GetCursorPos(&pt);
		ScreenToClient(hwnd_, &pt);

		browser->GetHost()->DragSourceEndedAt(
			client::DeviceToLogical(pt.x, device_scale_factor_),
			client::DeviceToLogical(pt.y, device_scale_factor_),
			result);
		browser->GetHost()->DragSourceSystemDragEnded();
		return true;
#else
		// Cancel the drag. The dragging implementation requires ATL support.
		return false;
#endif
	}

	void CPluginHTML5::UpdateDragCursor(
		CefRefPtr<CefBrowser> browser,
		CefRenderHandler::DragOperation operation) {
		CEF_REQUIRE_UI_THREAD();

#if defined(CEF_USE_ATL)
		current_drag_op_ = operation;
#endif
	}

#if defined(CEF_USE_ATL)

	CefBrowserHost::DragOperationsMask
		CPluginHTML5::OnDragEnter(CefRefPtr<CefDragData> drag_data,
			CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) {
		if (browser_) {
			client::DeviceToLogical(ev, device_scale_factor_);
			browser_->GetHost()->DragTargetDragEnter(drag_data, ev, effect);
			browser_->GetHost()->DragTargetDragOver(ev, effect);
		}
		return current_drag_op_;
	}

	CefBrowserHost::DragOperationsMask
		CPluginHTML5::OnDragOver(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) {
		if (browser_) {
			client::DeviceToLogical(ev, device_scale_factor_);
			browser_->GetHost()->DragTargetDragOver(ev, effect);
		}
		return current_drag_op_;
	}

	void CPluginHTML5::OnDragLeave() {
		if (browser_)
			browser_->GetHost()->DragTargetDragLeave();
	}

	CefBrowserHost::DragOperationsMask
		CPluginHTML5::OnDrop(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) {
		if (browser_) {
			client::DeviceToLogical(ev, device_scale_factor_);
			browser_->GetHost()->DragTargetDragOver(ev, effect);
			browser_->GetHost()->DragTargetDrop(ev);
		}
		return current_drag_op_;
	}

	CefBrowserHost::DragOperationsMask
		CPluginHTML5::OnDragOver(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) {
		if (browser_) {
			client::DeviceToLogical(ev, device_scale_factor_);
			browser_->GetHost()->DragTargetDragOver(ev, effect);
		}
		return current_drag_op_;
	}

	void CPluginHTML5::OnDragLeave() {
		if (browser_)
			browser_->GetHost()->DragTargetDragLeave();
	}

	CefBrowserHost::DragOperationsMask
		CPluginHTML5::OnDrop(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) {
		if (browser_) {
			client::DeviceToLogical(ev, device_scale_factor_);
			browser_->GetHost()->DragTargetDragOver(ev, effect);
			browser_->GetHost()->DragTargetDrop(ev);
		}
		return current_drag_op_;
	}

#endif  // defined(CEF_USE_ATL)

}  // namespace client
