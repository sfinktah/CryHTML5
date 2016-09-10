/* CryHTML5 - for licensing and copyright see license.txt */
#include "../../../../../AuthorityProjectConfig.h"

#include <StdAfx.h>
#include <CPluginHTML5.h>

#include <CEFHandler.hpp>
#include <CEFCryPak.hpp>
#include <CPluginD3D.h>
#include <OsrWndProc.h>

#include <PMUtils.hpp>
#include "TwSimpleDX11.h"
#include "../../../../../Sfinktah/debug.h"

#ifdef ENABLE_TEARLESS
#pragma comment(lib, "libcef")
#pragma comment(lib, "libcef_dll_wrapper")
#endif

namespace HTML5Plugin
{
    CPluginHTML5* gPlugin = NULL;
	CWndProc* gWndProc = NULL;
    D3DPlugin::IPluginD3D* gD3DSystem = NULL;

    CPluginHTML5::CPluginHTML5() :
        m_refCEFHandler(nullptr),
        m_refCEFRequestContext(nullptr),
        m_refCEFFrame(nullptr)
    {
        gPlugin = this;
	    gWndProc = new CWndProc;
        gD3DSystem = nullptr;
    }

	CPluginHTML5::CPluginHTML5(CPluginHTML5 *source) {

	}

    CPluginHTML5::~CPluginHTML5()
    {
        Release(true);
		gWndProc = NULL;

        gPlugin = NULL;
    }

    bool CPluginHTML5::Release(bool bForce)
    {

        bool bRet = true;
        bool bWasInitialized = m_bIsFullyInitialized; // Will be reset by base class so backup

        if (!m_bCanUnload)
        {
            // Note: Type Unregistration will be automatically done by the Base class (Through RegisterTypes)
            // Should be called while Game is still active otherwise there might be leaks/problems
            bRet = CPluginBase::Release(bForce);

            if (bRet)
            {
                ShutdownDependencies();

                if (bWasInitialized)
                {
                    // TODO: Cleanup stuff that can only be cleaned up if the plugin was initialized

                }

                // Cleanup like this always (since the class is static its cleaned up when the dll is unloaded)
                gPluginManager->UnloadPlugin(GetName());

                // Allow Plugin Manager garbage collector to unload this plugin
                AllowDllUnload();
            }
        }

        return bRet;
    };

    bool CPluginHTML5::Init(SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory)
    {
        bool bSuccess = true;

        // gPluginManager = (PluginManager::IPluginManager*)pPluginManager->GetConcreteInterface(NULL);
        // bSuccess = CPluginBase::Init(env, startupParams, pPluginManager, sPluginDirectory);

        return bSuccess;
    }

    void Command_DevTools(IConsoleCmdArgs* pArgs)
    {
        gPlugin->ShowDevTools();
    };

    void Command_URL(IConsoleCmdArgs* pArgs)
    {
#ifdef WE_HAD_A_CONSOLE
        if (pArgs->GetArgCount() > 1)
        {
#undef GetCommandLine
            string sText(pArgs->GetCommandLine());

            // read over the parameters
            size_t nOffset = sText.find_first_of(' ') + 1;

            // remove parameters
            sText = sText.Mid(nOffset).Trim();

            // delay the command
            if (sText.length() > 0)
            {
                gPlugin->SetURL(PluginManager::UTF82UCS2(sText));
            }
        }
    };

    void Command_JS(IConsoleCmdArgs* pArgs)
    {
        if (pArgs->GetArgCount() > 1)
        {
#undef GetCommandLine
            string sText(pArgs->GetCommandLine());

            // read over the parameters
            size_t nOffset = sText.find_first_of(' ') + 1;

            // remove parameters
            sText = sText.Mid(nOffset).Trim();

            // delay the command
            if (sText.length() > 0)
            {
                gPlugin->ExecuteJS(PluginManager::UTF82UCS2(sText));
            }
        }
#endif
    };

    void Command_Input(IConsoleCmdArgs* pArgs)
    {
#ifdef WE_HAD_A_CONSOLE
        if (pArgs->GetArgCount() == 2)
        {
            gPlugin->SetInputMode(PluginManager::ParseString<int>(pArgs->GetArg(1)));
        }

        else if (pArgs->GetArgCount() == 3)
        {
            gPlugin->SetInputMode(PluginManager::ParseString<int>(pArgs->GetArg(1)), PluginManager::ParseString<bool>(pArgs->GetArg(2)));
        }
#endif
    };

    bool CPluginHTML5::RegisterTypes(int nFactoryType, bool bUnregister)
    {
        bool bRet = TRUE; // or false, who knows
#ifdef WE_CARED
                          // Note: Autoregister Flownodes will be automatically registered by the Base class
        bool bRet = CPluginBase::RegisterTypes(nFactoryType, bUnregister);

        using namespace PluginManager;
        eFactoryType enFactoryType = eFactoryType(nFactoryType);

        if (bRet)
        {
            if (gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting())
            {
                // CVars
                if (gEnv->pConsole && (enFactoryType == FT_All || enFactoryType == FT_CVar))
                {
                    if (!bUnregister)
                    {
                        REGISTER_CVAR(cm5_active, 1.0f, VF_NULL, "CryHTML5 Rendering and systems active");
                        REGISTER_CVAR(cm5_alphatest, 0.3f, VF_NULL, "CryHTML5 Alpha test threshold for cursor");
                    }

                    else
                    {
                        gEnv->pConsole->UnregisterVariable("cm5_active", true);
                        gEnv->pConsole->UnregisterVariable("cm5_alphatest", true);
                    }
                }

                // CVars Commands
                if (gEnv->pConsole && (enFactoryType == FT_All || enFactoryType == FT_CVarCommand))
                {
                    if (!bUnregister)
                    {
                        gEnv->pConsole->AddCommand("cm5_devtools", Command_DevTools, VF_NULL, "Open the CryHTML5 DevTools");
                        gEnv->pConsole->AddCommand("cm5_url", Command_URL, VF_NULL, "Open the URL");
                        gEnv->pConsole->AddCommand("cm5_js", Command_JS, VF_NULL, "Execute the JavaScript");
                        gEnv->pConsole->AddCommand("cm5_input", Command_Input, VF_NULL, "Set Input mode");
                    }

                    else
                    {
                        gEnv->pConsole->RemoveCommand("cm5_devtools");
                        gEnv->pConsole->RemoveCommand("cm5_url");
                        gEnv->pConsole->RemoveCommand("cm5_js");
                        gEnv->pConsole->RemoveCommand("cm5_input");
                    }
                }
            }
        }
#endif
        return bRet;
    }

    const char* CPluginHTML5::GetStatus() const
    {
        return "OK";
    }

    const char* CPluginHTML5::ListCVars() const
    {
        return "..."; // TODO: Enter CVARs/Commands here if you have some
    }


    bool CPluginHTML5::CheckDependencies()const
    {
        //Check For The Existence Of All Dependencies Here.
        bool bSuccess = CPluginBase::CheckDependencies();

        if (bSuccess)
        {
            bSuccess = !!PluginManager::safeGetPluginConcreteInterface<D3DPlugin::IPluginD3D*>("D3D");
        }

        return bSuccess;
    }

    bool CPluginHTML5::InitDependencies()
    {
        //Initialize All Dependencies Here.
        bool bSuccess = true;

        bSuccess = InitD3DPlugin();

        HTML5Plugin::gPlugin->LogAlways("InitD3DPlugin %s ", bSuccess ? "success" : "failed");

        if (bSuccess)
        {
            bSuccess = InitializeCEF();
        }

        if (bSuccess)
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
        if (!gD3DSystem)
        {
            gPlugin->LogError("CPluginHTML5::InitD3DPlugin(): Failed To Get The D3D Plug-in (Plugin_D3D). This Plug-in (%s) Depends On The D3D Plug-in (Plugin_D3D).", GetName());
            return false;
        }

        else
        {
            /* These three re-added from henry's source, should um CHECK THEM again */
			/* So turns out we don't have these globally defined. */
			/* (confusion) they're in TwSimpleDX11, added header, and we'll see what happens */
            gD3DSystem->SetDevice(g_D3DDev);
            gD3DSystem->SetDeviceContext(g_D3DDevCtx);
            gD3DSystem->SetSwapChain(g_SwapChain);
            /* end added 3 */

            gD3DSystem->GetDevice(); // start search if isn't already found
        }

        return true;
    }

    void CPluginHTML5::ShutdownD3DPlugin()
    {
        if (gD3DSystem)
        {
            // Do not release the listeners !!...

            //Un-Registers This Instance From The D3D Plug-in.
            PluginManager::safeReleasePlugin("D3D", gD3DSystem);
        }
    }

    bool CPluginHTML5::InitializeCEF()
    {
        // Initialize Paths
        m_sCEFBrowserProcess = PluginManager::pathWithSeperator(gPluginManager->GetPluginDirectory(GetName())) + "cefclient.exe";
        m_sCEFLog = PluginManager::pathWithSeperator(gPluginManager->GetDirectoryRoot()) + "CryHTML5.log";
        m_sCEFResourceDir = gPluginManager->GetPluginDirectory(GetName());
        m_sCEFLocalesDir = PluginManager::pathWithSeperator(gPluginManager->GetPluginDirectory(GetName())) + "locales";
// Initialize Settings
        // Initialize Settings
        CefSettings settings;
        CefString(&settings.browser_subprocess_path).FromASCII(m_sCEFBrowserProcess.c_str());
        CefString(&settings.log_file).FromASCII(m_sCEFLog.c_str());
        CefString(&settings.resources_dir_path).FromASCII(m_sCEFResourceDir.c_str());
        CefString(&settings.locales_dir_path).FromASCII(m_sCEFLocalesDir.c_str());

        DEBUG_VAR_CSTR(m_sCEFBrowserProcess.c_str());
        DEBUG_VAR_CSTR(m_sCEFLog.c_str());
        DEBUG_VAR_CSTR(m_sCEFResourceDir.c_str());
        DEBUG_VAR_CSTR(m_sCEFLocalesDir.c_str());


        settings.command_line_args_disabled = true;
        settings.multi_threaded_message_loop = true;
        settings.log_severity = LOGSEVERITY_WARNING;
        settings.remote_debugging_port = 7574;
        settings.background_color = CefColorSetARGB(0, 0, 0, 0);

        // Now initialize CEF
        CefMainArgs main_args;
        // bool bSuccess = CefInitialize(main_args, settings, m_app, NULL);
        bool bSuccess = CefInitialize(main_args, settings, NULL, NULL);
        HTML5Plugin::gPlugin->LogAlways("Initialize %s ", bSuccess ? "success" : "failed");

        // Initialize Components
        if (bSuccess)
        {
            CefRegisterSchemeHandlerFactory("cry", "cry", new CEFCryPakHandlerFactory());

            // Initialize a Browser
            bSuccess = InitializeCEFBrowser();
        }

        return bSuccess;
    }

    bool CPluginHTML5::InitializeCEFBrowser()
    {
        // Client Handler
        m_refCEFHandler = new CEFCryHandler(TEARLESS_WINDOW_WIDTH, TEARLESS_WINDOW_HEIGHT);

        // Window Information
        CefWindowInfo info;

        info.transparent_painting_enabled = TRUE;
        info.windowless_rendering_enabled = TRUE;

        // info.SetAsOffScreen( HWND( gEnv->pRenderer->GetHWND() ) ); //info.SetAsOffScreen( NULL );
        // info.SetTransparentPainting( TRUE );

        // Register Listener for Render Handler
        gD3DSystem->RegisterListener(m_refCEFHandler->_renderHandler.get());

        // Browser Settings
        CefBrowserSettings browserSettings;
        //browserSettings.accelerated_compositing = STATE_ENABLED; // For OSR always software is used this is an CEF restriction.
        //browserSettings.webgl = STATE_ENABLED;

        m_refCEFRequestContext = CefRequestContext::GetGlobalContext();

        bool bSuccess = CefBrowserHost::CreateBrowser(info, m_refCEFHandler.get(), "cry://UI/TestUI.html", browserSettings, nullptr);
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, m_refCEFHandler.get(), "http://www.youtube.com/watch?v=3MteSlpxCpo", browserSettings, nullptr );
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, m_refCEFHandler.get(), "http://webglsamples.googlecode.com/hg/aquarium/aquarium.html", browserSettings, nullptr );
        //bool bSuccess = CefBrowserHost::CreateBrowser( info, g_handler.get(), "http://www.google.com", browserSettings, nullptr );

        HTML5Plugin::gPlugin->LogAlways("CreateBrowser %s", bSuccess ? "success" : "failed");
        return bSuccess;
    }

    void CPluginHTML5::ShowDevTools()
    {
        if (!m_sCEFDebugURL.empty())
        {
            LaunchExternalBrowser(m_sCEFDebugURL);
        }
    }

    void CPluginHTML5::LaunchExternalBrowser(const string& url)
    {
        if (CefCurrentlyOn(TID_PROCESS_LAUNCHER))
        {
            // Retrieve the current executable path.
            CefString file_exe(gPlugin->m_sCEFBrowserProcess);

            // Create the command line.
            CefRefPtr<CefCommandLine> command_line =
                CefCommandLine::CreateCommandLine();
            command_line->SetProgram(file_exe);
            command_line->AppendSwitchWithValue("url", url.c_str());

            // Launch the process.
            CefLaunchProcess(command_line);
        }

        else
        {
            // Execute on the PROCESS_LAUNCHER thread.
            CefPostTask(TID_PROCESS_LAUNCHER, NewCefRunnableFunction(&CPluginHTML5::LaunchExternalBrowser, url));
        }
    }

    void CPluginHTML5::ShutdownCEF()
    {
        gPlugin->LogAlways("Shutting down");

        /* XXX: This could be important for window binding */
		/* Actually it's only input handlers, like input devices i think */
        //m_refCEFHandler->m_input.UnregisterListeners();

        m_refCEFFrame = nullptr;
        m_refCEFHandler = nullptr;
        m_refCEFRequestContext = nullptr;

        CefShutdown();

        gPlugin->LogAlways("Closed");
    }

    bool CPluginHTML5::SetURL(const wchar_t* sURL)
    {
        if (m_refCEFFrame.get() != nullptr)
        {
            m_refCEFFrame->LoadURL(sURL);
            return true;
        }

        return false;
    }

    bool CPluginHTML5::ExecuteJS(const wchar_t* sJS)
    {
        if (m_refCEFFrame.get() != nullptr)
        {
            m_refCEFFrame->ExecuteJavaScript(sJS, CefString("CryHTML"), 0);
            return true;
        }

        return false;
    }

#ifdef WE_CARED
    bool CPluginHTML5::WorldPosToScreenPos(CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset /*= Vec3( ZERO ) */)
    {
        if (m_refCEFHandler.get() != nullptr && m_refCEFHandler->_renderHandler.get() != nullptr)
        {
            // get current camera matrix
            const Matrix34& camMat = cam.GetMatrix();

            // add offset to position
            const Vec3 vFaceingPos = camMat.GetTranslation() - camMat.GetColumn1() * 1000.f;
            const Vec3 vDir = (vWorld - vFaceingPos).GetNormalizedFast();
            const Vec3 vOffsetX = vDir.Cross(Vec3Constants<float>::fVec3_OneZ).GetNormalizedFast() * vOffset.x;
            const Vec3 vOffsetY = vDir * vOffset.y;
            const Vec3 vOffsetZ = Vec3(0, 0, vOffset.z);
            vWorld += vOffsetX + vOffsetY + vOffsetZ;

            // calculate screen x,y coordinates
            cam.SetMatrix(camMat);
            cam.Project(vWorld, vScreen);
            ScaleCoordinates(vScreen.x, vScreen.y, vScreen.x, vScreen.y, false, true);

            // store depth in z
            vScreen.z = (camMat.GetTranslation() - vWorld).GetLength();

            return true;
        }

        return false;
    }
#endif

    void CPluginHTML5::ScaleCoordinates(float fX, float fY, float& foX, float& foY, bool bLimit /*= false*/, bool bCERenderer /*= true */)
    {
        if (m_refCEFHandler.get() != nullptr)
        {
            m_refCEFHandler->_renderHandler->ScaleCoordinates(fX, fY, foX, foY, bLimit, bCERenderer);
        }
    }

    void CPluginHTML5::SetInputMode(int nMode, bool bExclusive)
    {
        if (m_refCEFHandler.get() != nullptr)
        {
            // m_refCEFHandler->m_input.SetInputMode(nMode, bExclusive);
        }
    }

    void CPluginHTML5::SetActive(bool bActive)
    {
        cm5_active = bActive ? 1.0f : 0.0f;
    }

    bool CPluginHTML5::IsCursorOnSurface()
    {
        if (cm5_active > 0.0 && m_refCEFHandler.get() != nullptr)
        {
            float fX = 0.0f, fY = 0.0f;
            // m_refCEFHandler->m_input.GetCursorPos(fX, fY);

            return IsOpaque(fX, fY);
        }

        return false;
    }

    bool CPluginHTML5::IsOpaque(float fX, float fY)
    {
#ifdef TEARLESS_TODO
        if (cm5_active > 0.0 && m_refCEFHandler.get() != nullptr)
        {
            const ColorB& color = m_refCEFHandler->_renderHandler->GetPixel(fX, fY);

            return color.a >= (cm5_alphatest * 255);
        }
#endif

        return false;
    }

}