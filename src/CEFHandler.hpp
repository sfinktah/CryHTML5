/* CryHTML5 - for licensing and copyright see license.txt */

#pragma once
#pragma warning(default:4263 4264 4265 4266 4623 4624 4625 4626)
#include <cef_app.h>
#include <cef_client.h>
#include <include/capi/cef_app_capi.h>
#include <include/capi/cef_render_process_handler_capi.h>

#include <CPluginHTML5.h>
#include <CEFRenderHandler.hpp>
#include "../../../../../Sfinktah/debug.h"

// #include <CEFInputHandler.hpp>

class CEFCryHandler; // : public CefClient, public CefLifeSpanHandler, public CefContextMenuHandler, public CefDialogHandler, public CefJSDialogHandler;
/** @brief handle loading of web pages */
class CEFCryLoadHandler : public CefLoadHandler
{
    public:
		
		//CefRefPtr<CefMessageRouterRendererSide> m_MessageRouterRenderSide = NULL;
        CefRefPtr<CEFCryHandler> _cefClient; //!< the renderer handler
        virtual void OnLoadingStateChange( CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward )
        {
            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadingStateChange: %s", SAFESTR( url.c_str() ) );
        }

        virtual void OnLoadStart( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame )
        {
            // A single CefBrowser instance can handle multiple requests for a single URL if there are frames (i.e. <FRAME>, <IFRAME>).
            //if ( frame->IsMain() )

            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadStart: %s", SAFESTR( url.c_str() ) );
        }

        virtual void OnLoadEnd( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode )
        {

            std::string url = browser->GetMainFrame()->GetURL().ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "LoadEnd: %s, %d", SAFESTR( url.c_str() ), httpStatusCode );
			//DEBUG_PTR(HTML5Plugin::gPlugin->m_refCEFHandler);
			DEBUG_PTR(HTML5Plugin::gPlugin->m_refCEFBrowser);
			DEBUG_PTR(_cefClient);
			// THis we tried last
			//_cefClient->SendContextCreated(HTML5Plugin::gPlugin->m_refCEFBrowser);

			//DEBUG_PTR(_renderHandler->SendContextCreated);
			HTML5Plugin::gPlugin->browser(browser); // remember the fucking browser why-not (sfink) -- should this have a CefRefPtr though?
			//HTML5Plugin::gPlugin->m_refCEFHandler->SendContextCreated(HTML5Plugin::gPlugin->m_refCEFBrowser);
        }

        virtual void OnLoadError( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl )
        {
            std::string url = browser->GetMainFrame()->GetURL().ToString();
            std::string furl = failedUrl.ToString();
            std::string err = errorText.ToString();
            HTML5Plugin::gPlugin->LogError( "LoadError: %s, %s, %d, %s", SAFESTR( url.c_str() ), SAFESTR( furl.c_str() ), int( errorCode ), SAFESTR( err.c_str() ) );
        }

        IMPLEMENT_REFCOUNTING( CEFCryLoadHandler );
};

/** @brief central CEF handler class per browser */
class CEFCryHandler : public CefClient, public CefLifeSpanHandler, public CefContextMenuHandler, public CefDialogHandler, public CefJSDialogHandler
{
    private:
        CefRefPtr<CEFCryLoadHandler> _loadHandler; //!< the load handler

    public:
        CefRefPtr<CEFCryRenderHandler> _renderHandler; //!< the renderer handler
		//CefRefPtr<CefMessageRouterRendererSide> m_MessageRouterRenderSide = NULL;
        //CEFCryInputHandler m_input; //!< the input handler
		//DelegateSet delegates_;


    public:
        CEFCryHandler( int windowWidth, int windowHeight )
        {
            _renderHandler = new CEFCryRenderHandler( windowWidth, windowHeight );
            _loadHandler = new CEFCryLoadHandler();
			_loadHandler->_cefClient = this;
			
			// IS it too early for this? 
			//CreateDelegates(delegates_);

        };

        ~CEFCryHandler() { };

#pragma warning(disable: 4927)

        // CefClient methods.
        virtual CefRefPtr<CefLoadHandler> GetLoadHandler()
        {
            return _loadHandler;
        }

        virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
        {
            return _renderHandler;
        }

        // CefContextMenuHandler
        virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler()
        {
            return this;
        }

        virtual void OnBeforeContextMenu( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model )
        {
            model->Clear(); // Never show context menus
        }

        // CefDialog Handler
        virtual CefRefPtr<CefDialogHandler> GetDialogHandler()
        {
            return this;
        }

        virtual bool OnFileDialog( CefRefPtr<CefBrowser> browser, FileDialogMode mode, const CefString& title, const CefString& default_file_name, const std::vector<CefString>& accept_types, CefRefPtr<CefFileDialogCallback> callback )
        {
            callback->Cancel();
            return true;
        }

        // CefJSDialogHandler
        virtual CefRefPtr<CefJSDialogHandler> GetJSDialogHandler()
        {
            return this;
        }

        virtual bool OnJSDialog( CefRefPtr<CefBrowser> browser, const CefString& origin_url, const CefString& accept_lang, JSDialogType dialog_type, const CefString& message_text, const CefString& default_prompt_text, CefRefPtr<CefJSDialogCallback> callback, bool& suppress_message )
        {
            string text = message_text.ToString().c_str();
            HTML5Plugin::gPlugin->LogAlways( "JSDialog: %s", text.c_str() );

            // suppress javascript messages 
            suppress_message = true;
            return false;
        }

        // CefLifeSpanHandler
        virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler()
        {
            return this;
        }

		virtual int SendContextCreated(CefRefPtr<CefBrowser> browser ) {
			DEBUG_PTR(browser);
			DEBUG_PTR(browser->GetMainFrame());
			DEBUG_PTR(browser->GetMainFrame()->GetV8Context());

			if (!browser->GetMainFrame()->GetV8Context()) {
				return 0;
			}

			DEBUG_OUT("m_MessageRouterRenderSide->OnContextCreated");
			//m_MessageRouterRenderSide->OnContextCrea ted(
			//	browser,
			//	browser->GetMainFrame(),
			//	browser->GetMainFrame()->GetV8Context());

			DEBUG_OUT("SendContextCreated");
		}

        virtual void OnAfterCreated( CefRefPtr<CefBrowser> browser )
        {
            if ( HTML5Plugin::gPlugin->m_sCEFDebugURL.empty() )
            {
                HTML5Plugin::gPlugin->m_refCEFFrame = browser->GetMainFrame(); // remember frame
                HTML5Plugin::gPlugin->m_refCEFBrowser  = browser; // remember the fucking browser why-not (sfink) -- should this have a CefRefPtr though?
                DEBUG_PTR(browser);
                DEBUG_PTR(browser->GetMainFrame());
				DEBUG_PTR(browser->GetMainFrame()->GetV8Context());
                // and why not call this event while we're fucking at it too
                //HTML5Plugin::gPlugin->OnAfterCreated(browser); // Surely we must need more CefRefPtr's

				//CefMessageRouterConfig config;
				//m_MessageRouterRenderSide = CefMessageRouterRendererSide::Create(config);
				//CefRefPtr<CefMessageRouterRendererSide> renderer_side_router_ = CefMessageRouterRendererSide::Create(config);
				/*
[Authority] CPluginHTML5::OnAfterCreated
[Authority] binding_test::Handler created
[Authority] AddHandler returned: 1
[Authority] binding_test::Handler created
[Authority] &_renderHandler: 0000028E06588040
[Authority] &_renderHandler->m_MessageRouterRenderSide: 0000000000000000
[Authority] m_MessageRouterRenderSide = _renderHandler->m_MessageRouterRenderSide
[Authority] ???????: 0000028E06588040, ????????????????????e: 0000000000000000
[Authority] &browser: 0000028E0660FA30
[Authority] &browser->GetMainFrame(): 0000028E0660FE70
[Authority] &browser->GetMainFrame()->GetV8Context(): 0000000000000000
[Authority] m_MessageRouterRenderSide->OnContextCreated
	*/ 

				DEBUG_PTR(_renderHandler);
				//DEBUG_PTR(_renderHandler->m_MessageRouterRenderSide);
				DEBUG_PTR(browser->GetMainFrame()->GetV8Context());
				DEBUG_OUT("m_MessageRouterRenderSide = _renderHandler->m_MessageRouterRenderSide");
				//ASSIGN_CHECK(
				//	m_MessageRouterRenderSide = _renderHandler->m_MessageRouterRenderSide,
				//	_renderHandler,
				//	_renderHandler->m_MessageRouterRenderSide
				//);


                // HTML5Plugin::gPlugin->m_sCEFDebugURL = browser->GetHost()->GetDevToolsURL( false ).ToString().c_str(); // browser->GetHost()->GetDevToolsURL( false ).ToString();
                HTML5Plugin::gPlugin->LogAlways( "Devtools URL: %s", HTML5Plugin::gPlugin->m_sCEFDebugURL.c_str() );

                HTML5Plugin::gPlugin->ShowDevTools();
            }
        }; 

		virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser)
		{
			DEBUG_PTR(browser->GetMainFrame()->GetV8Context());
			//m_MessageRouterRenderSide->OnContextReleased(
			//	browser,
			//	browser->GetMainFrame(),
			//	browser->GetMainFrame()->GetV8Context()
			//);

            if ( HTML5Plugin::gPlugin->m_refCEFFrame.get() == nullptr || HTML5Plugin::gPlugin->m_refCEFFrame->GetBrowser()->GetIdentifier() == browser->GetIdentifier() )
            {
                HTML5Plugin::gPlugin->m_sCEFDebugURL = "";
                HTML5Plugin::gPlugin->m_refCEFFrame = nullptr;
                HTML5Plugin::gPlugin->m_refCEFBrowser = nullptr; // sfink: better remove it aswell
            }
        };

        virtual bool OnBeforePopup( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, bool* no_javascript_access )
        {
			DEBUG_PTR(browser->GetMainFrame());
			DEBUG_PTR(frame);
			DEBUG_PTR(windowInfo);
			DEBUG_PTR(client);
			DEBUG_PTR(browser->GetMainFrame()->GetV8Context());
			DEBUG_PTR(frame->GetV8Context());
			DEBUG_PTR(client->GetRenderHandler());
			DEBUG_PTR(client->GetRequestHandler());

            return true; // Never allow Popups
        }

        IMPLEMENT_REFCOUNTING( CEFCryHandler );

};

