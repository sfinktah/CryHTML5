/* CryHTML5 - for licensing and copyright see license.txt */

#define __OVERRIDE override
#pragma once

#include <IPluginBase.h>
// #include <Game.h>

// #include <IPluginManager.h>
#include <CPluginBase.hpp>

#include <IPluginHTML5.h>
#include <IPluginD3D.h>
#include <OsrWndProc.h>

#define PLUGIN_NAME "HTML5"
#define PLUGIN_CONSOLE_PREFIX "[" PLUGIN_NAME " " PLUGIN_TEXT "] " //!< Prefix for Logentries by this plugin

#include <cef_app.h>
#include <cef_client.h>

class CEFCryHandler;

namespace HTML5Plugin
{
    /**
     * @brief Provides information and manages the resources of this plugin.
     */
    class CPluginHTML5 :
        public PluginManager::CPluginBase,
        public IPluginHTML5
    {
        public:
            CPluginHTML5();
            ~CPluginHTML5();

            float cm5_active; //!< cvar to activate the plugin
            float cm5_alphatest; //!< cvar for alpha test check

            string m_sCEFBrowserProcess; //!< path to browser process
            string m_sCEFLog; //!< path to log file
            string m_sCEFResourceDir; //!< path to resource directory
            string m_sCEFLocalesDir;
            string m_sCEFDebugURL;

            CefRefPtr<CEFCryHandler> m_refCEFHandler;
            CefRefPtr<CefRequestContext> m_refCEFRequestContext;
            CefRefPtr<CefFrame> m_refCEFFrame;
            // XXX: Added vvv
            CefRefPtr<CefBrowser> m_refCEFBrowser; // sfink: why the fuck couldn't we just have this to begin with

            // IPluginBase
            bool Release(bool bForce = false)__OVERRIDE;

            int GetInitializationMode() const __OVERRIDE
            {
                return int(PluginManager::IM_Default);
            };

            bool Init(SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory)__OVERRIDE;

            bool RegisterTypes(int nFactoryType, bool bUnregister)__OVERRIDE;

            const char* GetVersion() const __OVERRIDE
            {
                return "0.5.0.0";
            };

            const char* GetName() const __OVERRIDE
            {
                return PLUGIN_NAME;
            };

            const char* GetCategory() const __OVERRIDE
            {
                return "Visual";
            };

            const char* ListAuthors() const __OVERRIDE
            {
                return "Hendrik Polczynski, Richard Marcoux III";
            };

            const char* ListCVars() const __OVERRIDE;

            const char* GetStatus() const __OVERRIDE;

            const char* GetCurrentConcreteInterfaceVersion() const __OVERRIDE
            {
                return "1.0";
            };

            void* GetConcreteInterface(const char* sInterfaceVersion) __OVERRIDE
            {
                return static_cast <IPluginHTML5*>(this);
            };

            virtual bool InitDependencies() __OVERRIDE;

            virtual bool CheckDependencies() const __OVERRIDE;

            // IPluginHTML5
            IPluginBase* GetBase() __OVERRIDE
            {
                return static_cast<IPluginBase*>(this);
            };

            // TODO: Add your concrete interface implementation
        private:

            /**
             * @brief Shuts Down This Instance's Plug-in Dependencies.
             * @return void
             */
            void ShutdownDependencies();

            void ShutdownCEF();

            /**
             * @brief Initializes The Dependent D3D Plug-in.  Called By This Instance's InitDependencies() Method.
             * @return True If The D3D Plug-in Was Successfully Initialized.  False Otherwise.
             */
            bool InitD3DPlugin();

            bool InitializeCEF();
            bool InitializeCEFBrowser();

            /**
             * @brief Shuts Down The Dependent D3D Plug-in.  Called By This Instance's ShutdownDependencies()
             */
            void ShutdownD3DPlugin();

        public:
            static void LaunchExternalBrowser(const string& url);
            void ShowDevTools();

            virtual bool SetURL(const wchar_t* sURL);

            virtual bool ExecuteJS(const wchar_t* sJS);

            //virtual bool WorldPosToScreenPos(CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset = Vec3(ZERO));

            virtual void ScaleCoordinates(float fX, float fY, float& foX, float& foY, bool bLimit = false, bool bCERenderer = true);

            virtual void SetInputMode(int nMode = 0, bool bExclusive = false);

            virtual void SetActive(bool bActive);

            virtual bool IsCursorOnSurface();

            virtual bool IsOpaque(float fX, float fY);

    };

	class CWndProc
	{
	public:
		static LRESULT CALLBACK OsrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		// from cef osr demo - but osr_widget_win bit
		static int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);
		static int GetCefMouseModifiers(WPARAM wparam);
		static bool isKeyDown(WPARAM wparam);

		// Show the popup window with correct parent and bounds in parent coordinates.
#ifdef TEARLESS_HANDLES_POPUPS
		void ShowPopup(HWND parent_hwnd, int x, int y, size_t width, size_t height);

		void Show();
		void Hide();
		void SetBounds(int x, int y, size_t width, size_t height);
		void SetFocus();
#endif
		void SetDeviceScaleFactor(float device_scale_factor_width, float device_scale_factor_height);



		// private:
			// Only allow deletion via scoped_refptr.
		//friend struct CefDeleteOnThread<TID_UI>;
		//friend class base::RefCountedThreadSafe<client::OsrWindowWin, CefDeleteOnUIThread>;


		// Manage native window lifespan.
		//void Create(HWND parent_hwnd, const RECT& rect);
		//void Destroy();

		// Manage GL context lifespan.
		void EnableGL();
		void DisableGL();

		// Redraw what is currently in the texture.
		//void Invalidate();
		void Render();

		void NotifyNativeWindowCreated(HWND hwnd);

		static void RegisterOsrClass(HINSTANCE hInstance,
			HBRUSH background_brush);
		// WndProc message handlers.
		void OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
		void OnSize();
		void OnSizing(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void OnFocus(bool setFocus);
		void OnCaptureLost();
		void OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam);
		void OnPaint();
		bool OnEraseBkgnd();

		// Manage popup bounds.
#ifdef TEARLESS_HAS_RENDERER
		bool IsOverPopupWidget(int x, int y) const;
		int GetPopupXOffset() const;
		int GetPopupYOffset() const;
		void ApplyPopupOffset(int& x, int& y) const;
#endif

	public:

		// The below members are only accessed on the UI thread.
		//OsrRenderer renderer_;
		HWND hwnd_;
		HDC hdc_;
		HGLRC hrc_;

		RECT client_rect_;
		float device_scale_factor_width_;
		float device_scale_factor_height_;

		CefRefPtr<CefBrowser> browser_;

#if defined(CEF_USE_ATL)
		CComPtr<DropTargetWin> drop_target_;
		CefRenderHandler::DragOperation current_drag_op_;
#endif

		bool painting_popup_;
		bool render_task_pending_;
		bool hidden_;

		// Mouse state tracking.
		POINT last_mouse_pos_;
		POINT current_mouse_pos_;
		bool mouse_rotation_;
		bool mouse_tracking_;
		int last_click_x_;
		int last_click_y_;
		CefBrowserHost::MouseButtonType last_click_button_;
		int last_click_count_;
		double last_click_time_;
		bool last_mouse_down_on_view_;

	};
	extern CWndProc* gWndProc;
    extern CPluginHTML5* gPlugin;
    extern D3DPlugin::IPluginD3D* gD3DSystem;
}

/**
 * @brief This function is required to use the Autoregister Flownode without modification.
 * Include the file "CPluginHTML5.h" in front of flownode.
 */
//inline void GameWarning(const char* sFormat, ...) PRINTF_PARAMS(1, 2);
//inline void GameWarning(const char* sFormat, ...)
//{
//    va_list ArgList;
//    va_start(ArgList, sFormat);
//    HTML5Plugin::gPlugin->LogV(FLog::eWarningAlways, sFormat, ArgList);
//    va_end(ArgList);
//};
