/* CryHTML5 - for licensing and copyright see license.txt */

#pragma once

#include <IPluginBase.h>
// #include <Game.h>

// #include <IPluginManager.h>
#include <CPluginBase.hpp>

#include <IPluginHTML5.h>
#include <IPluginD3D.h>

#define PLUGIN_NAME "HTML5"
#define PLUGIN_CONSOLE_PREFIX "[" PLUGIN_NAME " " PLUGIN_TEXT "] " //!< Prefix for Logentries by this plugin

#include <cef_app.h>
#include <cef_client.h>


// from cef osr demo

#include <include/base/cef_bind.h>
#include <include/base/cef_ref_counted.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>
#include <cefclient/browser/client_handler_osr.h>
#include <cefclient/browser/osr_dragdrop_win.h>
#include <cefclient/browser/osr_renderer.h>
#include <wrapper/cef_message_router.h>


// end from cef osr demo

#define __OVERRIDE override

#ifdef WHY_DOES_THIS_FUCK_ME_UP
#define _HAS_ITERATOR_DEBUGGING 1
#define _ITERATOR_DEBUG_LEVEL 2 
#define _SECURE_SCL 1
#endif

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

		// From client_handler.h
		CefRefPtr<CefMessageRouterBrowserSide> message_router_;
		// Manages the registration and delivery of resources.
		CefRefPtr<CefResourceManager> resource_manager_;
		typedef std::set<CefMessageRouterBrowserSide::Handler*> MessageHandlerSet;
		int browser_count_;
		// Set of Handlers registered with the message router.
		MessageHandlerSet message_handler_set_;
		// End from client_handler.h



		CPluginHTML5();
		~CPluginHTML5();
		float m_deviceScaleFactor;

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

		virtual bool WorldPosToScreenPos(CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset = Vec3(ZERO));

		virtual void ScaleCoordinates(float fX, float fY, float& foX, float& foY, bool bLimit = false, bool bCERenderer = true);

		virtual void SetInputMode(int nMode = 0, bool bExclusive = false);

		virtual void SetActive(bool bActive);

		virtual bool IsCursorOnSurface();

		virtual bool IsOpaque(float fX, float fY);


		// from cef osr demo - but osr_widget_win bit
		static int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);
		static int GetCefMouseModifiers(WPARAM wparam);
		static bool isKeyDown(WPARAM wparam);
		//bool IsOverPopupWidget(int x, int y) const;
		//int GetPopupXOffset() const;
		//int GetPopupYOffset() const;
		//void ApplyPopupOffset(int& x, int& y) const;

		// from cef osr demo, uncomment as needed

		// Show the popup window with correct parent and bounds in parent coordinates.
		void ShowPopup(HWND parent_hwnd, int x, int y, size_t width, size_t height);

		void Show();
		void Hide();
		void SetBounds(int x, int y, size_t width, size_t height);
		void SetFocus();
		void SetDeviceScaleFactor(float device_scale_factor_width, float device_scale_factor_height);

		static LRESULT CALLBACK OsrWndProc(HWND hWnd, UINT message, WPARAM wParam,
			LPARAM lParam);


	// private:
		// Only allow deletion via scoped_refptr.
		friend struct CefDeleteOnThread<TID_UI>;
		//friend class base::RefCountedThreadSafe<OsrWindowWin, CefDeleteOnUIThread>;

		//~OsrWindowWin();

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
		//bool IsOverPopupWidget(int x, int y) const;
		//int GetPopupXOffset() const;
		//int GetPopupYOffset() const;
		//void ApplyPopupOffset(int& x, int& y) const;

		// ClientHandlerOsr::OsrDelegate methods.
		void OnAfterCreated(CefRefPtr<CefBrowser> browser) /* OVERRIDE */;
		void AnotherMessageRouter();
		void CreateMessageRouter();
		void OnBeforeClose(CefRefPtr<CefBrowser> browser) /* OVERRIDE */;
		bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) /* OVERRIDE */;
		bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) /* OVERRIDE */;
		bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
			int viewX,
			int viewY,
			int& screenX,
			int& screenY) /* OVERRIDE */;
		bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) /* OVERRIDE */;
		void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) /* OVERRIDE */;
		void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) /* OVERRIDE */;
		void OnPaint(CefRefPtr<CefBrowser> browser,
			CefRenderHandler::PaintElementType type,
			const CefRenderHandler::RectList& dirtyRects,
			const void* buffer,
			int width,
			int height) /* OVERRIDE */;
		void OnCursorChange(CefRefPtr<CefBrowser> browser,
			CefCursorHandle cursor,
			CefRenderHandler::CursorType type,
			const CefCursorInfo& custom_cursor_info) /* OVERRIDE */;
		bool StartDragging(CefRefPtr<CefBrowser> browser,
			CefRefPtr<CefDragData> drag_data,
			CefRenderHandler::DragOperationsMask allowed_ops,
			int x, int y) /* OVERRIDE */;
		void UpdateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) /* OVERRIDE */;

#if defined(CEF_USE_ATL)
		// OsrDragEvents methods.
		CefBrowserHost::DragOperationsMask OnDragEnter(
			CefRefPtr<CefDragData> drag_data,
			CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) /* OVERRIDE */;
		CefBrowserHost::DragOperationsMask OnDragOver(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) /* OVERRIDE */;
		void OnDragLeave() /* OVERRIDE */;
		CefBrowserHost::DragOperationsMask OnDrop(CefMouseEvent ev,
			CefBrowserHost::DragOperationsMask effect) /* OVERRIDE */;
#endif  // defined(CEF_USE_ATL)

		// Only accessed on the main thread.
		// Delegate* delegate_;

		// The below members are only accessed on the UI thread.
		// OsrRenderer renderer_;
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

		//DISALLOW_COPY_AND_ASSIGN(OsrWindowWin);


	};

	extern CPluginHTML5* gPlugin;
	extern D3DPlugin::IPluginD3D* gD3DSystem;
	// extern CPluginD3D* gD3DSystem;

}

/**
 * @brief This function is required to use the Autoregister Flownode without modification.
 * Include the file "CPluginHTML5.h" in front of flownode.
 */
inline void GameWarning( const char* sFormat, ... ) PRINTF_PARAMS( 1, 2 );
inline void GameWarning( const char* sFormat, ... )
{
    va_list ArgList;
    va_start( ArgList, sFormat );
    HTML5Plugin::gPlugin->LogV( ILog::eWarningAlways, sFormat, ArgList );
    va_end( ArgList );
};
