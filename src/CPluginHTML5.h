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

            // IPluginBase
            bool Release( bool bForce = false )__OVERRIDE;

            int GetInitializationMode() const __OVERRIDE
            {
                return int( PluginManager::IM_Default );
            };

            bool Init( SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory )__OVERRIDE;

            bool RegisterTypes( int nFactoryType, bool bUnregister )__OVERRIDE;

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

            void* GetConcreteInterface( const char* sInterfaceVersion ) __OVERRIDE
            {
                return static_cast <IPluginHTML5*>( this );
            };

            virtual bool InitDependencies() __OVERRIDE;

            virtual bool CheckDependencies() const __OVERRIDE;

            // IPluginHTML5
            IPluginBase* GetBase() __OVERRIDE
            {
                return static_cast<IPluginBase*>( this );
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

            bool InitializeCEF( );
            bool InitializeCEFBrowser( );

            /**
             * @brief Shuts Down The Dependent D3D Plug-in.  Called By This Instance's ShutdownDependencies()
             */
            void ShutdownD3DPlugin();

        public:
            static void LaunchExternalBrowser( const string& url );
            void ShowDevTools();

            virtual bool SetURL( const wchar_t* sURL );

            virtual bool ExecuteJS( const wchar_t* sJS );

            virtual bool WorldPosToScreenPos( CCamera cam, Vec3 vWorld, Vec3& vScreen, Vec3 vOffset = Vec3( ZERO ) );

            virtual void ScaleCoordinates( float fX, float fY, float& foX, float& foY, bool bLimit = false, bool bCERenderer = true );

            virtual void SetInputMode( int nMode = 0, bool bExclusive = false );

            virtual void SetActive( bool bActive );

            virtual bool IsCursorOnSurface();

            virtual bool IsOpaque( float fX, float fY );

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
