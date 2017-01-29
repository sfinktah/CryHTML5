/* CryHTML5 - for licensing and copyright see license.txt */

#include <platform.h>
#include <ICryPak.h>
#include <PMUtils.hpp>

#include <cef_scheme.h>
#include <include/wrapper/cef_stream_resource_handler.h>
#include <Sfinktah/debug.h>
#include <CEFQueryUrl.h>


uint32_t reference_jenkins_one_at_a_time_hash(char *key, size_t len)
{
	uint32_t hash, i;
	for(hash = i = 0; i < len; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

constexpr unsigned int SHL(unsigned int hash, unsigned int n) {
	return (hash + (hash << n));
}
constexpr unsigned int SHR(unsigned int hash, unsigned int n) {
	return (hash + (hash >> n));
}
constexpr unsigned int XHR(unsigned int hash, unsigned int n) {
	return (hash ^ (hash >> n));
}

constexpr unsigned int JOAAT(const char* text, unsigned int hash = 0) {
	return *(text + 1) 
		? JOAAT((const char *)(text + 1), XHR(SHL((hash + *text), 10), 6))
		: SHL(XHR(SHL((XHR(SHL((hash + *text), 10), 6)), 3), 11), 15)
		;
}


// from http://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform
#ifndef URL_HH_
#define URL_HH_    
#include <string>
struct url {
	url(const std::string& url_s); // omitted copy, ==, accessors, ...
	std::string query() { return protocol_; }
private:
	void parse(const std::string& url_s);
private:
	std::string protocol_, host_, path_, query_;
};
#endif /* URL_HH_ */

// #include "url.hh"
#include <string>
#include <iterator>
#include <algorithm>
#include <cctype>
#include <functional>

void url::parse(const std::string& url_s)
{
	/* Protocol */
	const std::string prot_end("://");

	std::string::const_iterator prot_i = search(url_s.begin(), url_s.end(), prot_end.begin(), prot_end.end());

	protocol_.reserve(distance(url_s.begin(), prot_i));

	transform(url_s.begin(), prot_i,
		back_inserter(protocol_),
		std::ptr_fun<int,int>(tolower)); // protocol is icase

	if( prot_i == url_s.end() )
		return;

	advance(prot_i, prot_end.length());


	/* Path */
	std::string::const_iterator path_i = find(prot_i, url_s.end(), '/');

	host_.reserve(distance(prot_i, path_i));
	transform(prot_i, path_i,
		back_inserter(host_),
		std::ptr_fun<int,int>(tolower)); // host is icase

	/* Path & Query */
	std::string::const_iterator query_i = find(path_i, url_s.end(), '?');
	path_.assign(path_i, query_i);
	if( query_i != url_s.end() )
		++query_i;
	query_.assign(query_i, url_s.end());
}
// From http://www.gnu.org/software/cgicc
char hexToChar(char first,
	char second)
{
	int digit;

	digit = (first >= 'A' ? ((first & 0xDF) - 'A') + 10 : (first - '0'));
	digit *= 16;
	digit += (second >= 'A' ? ((second & 0xDF) - 'A') + 10 : (second - '0'));
	return static_cast<char>(digit);
}

std::string form_urldecode(const std::string& src)
{
	std::string result;
	std::string::const_iterator iter;
	char c;

	for(iter = src.begin(); iter != src.end(); ++iter) {
		switch(*iter) {
		case '+':
			result.append(1, ' ');
			break;
		case '%':
			// Don't assume well-formed input
			if(std::distance(iter, src.end()) >= 2
				&& std::isxdigit(*(iter + 1)) && std::isxdigit(*(iter + 2))) {
				c = *++iter;
				result.append(1, hexToChar(c, *++iter));
			}
			// Just pass the % through untouched
			else {
				result.append(1, '%');
			}
			break;

		default:
			result.append(1, *iter);
			break;
		}
	}

	return result;
}

/** @brief Implementation of the resource handler for client requests. */
class CEFCryPakResourceHandler : public CefResourceHandler
{
    public:
        FILE* m_fHandle; //!< file handle inside CryPak
		string m_sPath; //!< file path
        string m_sExtension; //!< file extension
		string m_sQuery; //<- optional query CefString
        string m_sMime; //!< mime type
		std::string m_sJsonReply;
		size_t m_nJsonReply;
        size_t m_nSize; //!< file size
        size_t m_nOffset; //!< current position inside of file

        CEFCryPakResourceHandler()
        {
            m_fHandle = NULL;
            m_sExtension = "html";
            m_nSize = 0;
            m_nOffset = 0;
        }

        /**
        * @brief Process Request
        * @param request request
        * @param callback callback
        */
		virtual bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) OVERRIDE
		{

			// Evaluate request to determine proper handling
			bool handled = false;

			// CryPak uses UTF-8
			string sPath = PluginManager::UCS22UTF8(request->GetURL().ToWString().c_str());

			// Get path
			size_t nOffset = sPath.find_first_of('/') + 2;
			m_sPath = sPath.Mid(nOffset).Trim();

			// Get extension
			nOffset = m_sPath.find_last_of('.') + 1;
			m_sExtension = m_sPath.Mid(nOffset, 5).Trim().MakeLower();

			// Get query
			nOffset = sPath.find_first_of('?') + 1;
			if (nOffset > 0) {
				m_sQuery = sPath.Mid(nOffset).Trim();
				const string query = m_sQuery;
				std::string decodedQuery = form_urldecode(query.c_str());
				//DEBUG_OUT_A("Decoded Query: %s", decodedQuery.c_str());
				m_sJsonReply = HTML5Plugin::CEFQueryUrl::ProcessQuery(request, callback, m_sPath.c_str(), m_sExtension.c_str(), decodedQuery.c_str());
				//DEBUG_OUT_A("JSON Reply: %s", m_sJsonReply.c_str());
				m_sMime = "application/json";
				m_nSize = m_sJsonReply.length();

                callback->Continue();
                return true; // Return true to handle the request.
			}

            // Get Mime (TODO: Later use CefGetMimeType -> requires update) // https://code.google.com/p/chromiumembedded/source/detail?r=1577
            switch (JOAAT(m_sExtension.c_str())) {
                case JOAAT("eot"):  m_sMime = "application/vnd.ms-fontobject";     break;
                case JOAAT("jpg"):  m_sMime = "image/jpeg";                        break;
                case JOAAT("png"):  m_sMime = "image/png";                         break;
                case JOAAT("js"):   m_sMime = "application/javascript";            break;
                case JOAAT("tiff"): m_sMime = "image/tiff";                        break;
                case JOAAT("bmp"):  m_sMime = "image/bmp";                         break;
                case JOAAT("gif"):  m_sMime = "image/gif";                         break;
                case JOAAT("cab"):  m_sMime = "application/vnd.ms-cab-compressed"; break;
                case JOAAT("css"):  m_sMime = "text/css";                          break;
                case JOAAT("xml"):  m_sMime = "application/xml";                   break;
                case JOAAT("exe"):  m_sMime = "application/x-msdownload";          break;
                case JOAAT("ico"):  m_sMime = "image/x-icon";                      break;
                case JOAAT("psf"):  m_sMime = "application/x-font-linux-psf";      break;
                case JOAAT("html"): m_sMime = "text/html";                         break;
                case JOAAT("msi"):  m_sMime = "application/x-msdownload";          break;
                case JOAAT("svg"):  m_sMime = "image/svg+xml";                     break;
                case JOAAT("mar"):  m_sMime = "application/octet-stream";          break;
                case JOAAT("txt"):  m_sMime = "text/plain";                        break;
                case JOAAT("json"): m_sMime = "application/json";                  break;
                case JOAAT("woff"): m_sMime = "application/x-font-woff";           break;
                case JOAAT("woff2"):m_sMime = "font/woff2";						   break;
                case JOAAT("jpeg"): m_sMime = "image/jpeg";                        break;
                case JOAAT("zip"):  m_sMime = "application/zip";                   break;
                case JOAAT("htm"):  m_sMime = "text/html";                         break;
                case JOAAT("bz2"):  m_sMime = "application/x-bzip2";               break;
                case JOAAT("ttf"):  m_sMime = "application/x-font-ttf";            break;
                case JOAAT("crt"):  m_sMime = "application/x-x509-ca-cert";        break;
                case JOAAT("mp4"):  m_sMime = "video/mp4";                         break;
                case JOAAT("svc"):  m_sMime = "application/vnd.dvb.service";       break;
                case JOAAT("pl"):   m_sMime = "text/plain";                        break;
                case JOAAT("swf"):  m_sMime = "application/x-shockwave-flash";     break;
                case JOAAT("pdf"):  m_sMime = "application/pdf";                   break;
                case JOAAT("cer"):  m_sMime = "application/pkix-cert";             break;
                case JOAAT("deb"):  m_sMime = "application/x-debian-package";      break;
                case JOAAT("atom"): m_sMime = "application/atom+xml";              break;
                case JOAAT("aac"):  m_sMime = "audio/x-aac";                       break;
                case JOAAT("m3u8"): m_sMime = "application/vnd.apple.mpegurl";     break;
                case JOAAT("com"):  m_sMime = "application/x-msdownload";          break;
                case JOAAT("mp3"):  m_sMime = "audio/mpeg";                        break;
                case JOAAT("crl"):  m_sMime = "application/pkix-crl";              break;
                case JOAAT("otf"):  m_sMime = "application/x-font-otf";            break;
				default:             m_sMime = "application/octet-stream";
            }

            // Open File
			// sfink
            // if ( ( m_fHandle = gEnv->pCryPak->FOpen( m_sPath, "rb" ) ) == NULL )
			if ((m_fHandle = ::fopen(m_sPath, "rb")) == NULL)
			{
				HTML5Plugin::gPlugin->LogWarning("ProcessRequest(%s) Unable to find specified path in pak", m_sPath.c_str());
			}

            else {
				m_nSize;
				fseek(m_fHandle, 0L, SEEK_END);
				m_nSize = ftell(m_fHandle);
				// fseek(m_fHandle, 0L, SEEK_SET);
				rewind(m_fHandle);
                // m_nSize = gEnv->pCryPak->FGetSize( m_fHandle );

                HTML5Plugin::gPlugin->LogAlways( "ProcessRequest(%s) Success Ext(%s) Mime(%s) Size(%ld)", m_sPath.c_str(), m_sExtension.c_str(), m_sMime.c_str(), m_nSize );

                // Indicate the headers are available.
                callback->Continue();
                return true; // Return true to handle the request.
            }

            return false;
        }

        /**
        * @brief get standard response header
        * @param response response
        * @param response_length response_length
        * @param redirectUrl redirectUrl
        */
        virtual void GetResponseHeaders( CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl ) OVERRIDE
        {

            if(m_fHandle || m_sJsonReply.length())
            {
                // Populate the response headers.
                response->SetStatus( 200 ); // OK
                response->SetMimeType( m_sMime.c_str() );
                // Specify the resulting response length.
                response_length = m_nSize;
            } else {
                response->SetStatus( 404 ); // not found
                response->SetMimeType( m_sMime.c_str() );
                // Specify the resulting response length.
                response_length = 0;
            }
        }

        virtual void Cancel() OVERRIDE
        {
            if ( m_fHandle )
            {
                gEnv->pCryPak->FClose( m_fHandle );
                m_fHandle = NULL;
            }
        }

        /**
        * @brief read data from disk
        * @param data_out data_out
        * @param bytes_to_read bytes_to_read
        * @param bytes_read bytes_read
        * @param callback callback
        * @return data left?
        */
        virtual bool ReadResponse( void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback ) OVERRIDE
        {
            // Read up to |bytes_to_read| data into |data_out| and set |bytes_read|.
            // If data isn't immediately available set bytes_read=0 and execute
            // |callback| asynchronously.
            // Return true to continue the request or false to complete the request.
            bool has_data = false;
            bytes_read = 0;

			//DEBUG_OUT_A("bytes_read: %i, bytes_to_read: %i, m_nSize: %i, m_nOffset: %i",
			//	bytes_read,
			//	bytes_to_read,
			//	m_nSize,
			//	m_nOffset);

			if (m_nOffset < m_nSize && data_out && (m_fHandle || m_sJsonReply.length()))
            {
                // Copy the next block of data into the buffer.
                int transfer_size = min( bytes_to_read, static_cast<int>( m_nSize - m_nOffset ) );
				//DEBUG_OUT_A("transfer_size: %i", transfer_size);
			

				if (m_sJsonReply.length()) {
					char *copyout;
					// transfer_size = strlen(copyout = strncpy((char *)data_out, &m_sJsonReply[m_nOffset], bytes_to_read));
					// XXX sfink: `m_nSize` can be bigger than the buffer and will cause overflow, replaced with `transfer_size`
					copyout = strncpy((char *)data_out, m_sJsonReply.c_str(), /* m_nSize */ transfer_size);
					bytes_read = transfer_size;
					//DEBUG_OUT_A("transfer_size: %i, content: %s", transfer_size, m_sJsonReply.c_str());
					return true;
				}
				else {
					// Read from pack
					transfer_size = ::fread(data_out, 1, bytes_to_read, m_fHandle);
					// transfer_size = gEnv->pCryPak->FReadRaw( data_out, 1, transfer_size, m_fHandle );
				}

                // Save offset
                m_nOffset += transfer_size;
				//DEBUG_OUT_A("m_nOffset: %i", m_nOffset);

                // Success
                bytes_read = transfer_size;
				//DEBUG_OUT_A("bytes_read: %i", bytes_read);
                has_data = true; // m_nOffset < m_nSize ?
            }

			//DEBUG_OUT_A("has_data: %i", has_data);
            // Close File Handle
            if ( !has_data )
            {
				//DEBUG_OUT_A("Cancelled()\n");
                Cancel();
            }

            return has_data;
        }

    private:
        IMPLEMENT_REFCOUNTING( CEFCryPakResourceHandler );
};

/** @brief Implementation of the factory for creating client request handlers. */
class CEFCryPakHandlerFactory : public CefSchemeHandlerFactory
{
    public:
        virtual CefRefPtr<CefResourceHandler> Create( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request ) OVERRIDE
        {
			//DEBUG_OUT("CEFCryPackhandlerFactory::Create");
			//DEBUG_PTR(browser);
			//DEBUG_PTR(browser->GetMainFrame());
			//DEBUG_PTR(browser->GetMainFrame()->GetV8Context());
            // Return a new resource handler instance to handle the request.
            return new CEFCryPakResourceHandler();

            /*
            // Create a stream reader for |html_content|.
            CefRefPtr<CefStreamReader> stream =
                CefStreamReader::CreateForData(
                static_cast<void*>(const_cast<char*>(html_content.c_str())),
                html_content.size());

            // Constructor for HTTP status code 200 and no custom response headers.
            // There’s also a version of the constructor for custom status code and response headers.
            return new CefStreamResourceHandler("text/html", stream);
            */
        }

        IMPLEMENT_REFCOUNTING( CEFCryPakHandlerFactory );
};
