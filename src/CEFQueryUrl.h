// #pragma once
#include <platform.h>
//
//#include "include/cef_base.h"
//#include "include/cef_browser.h"
//#include "include/cef_frame.h"
//#include "include/cef_request.h"
//#include "include/cef_response.h"
//#include "include/cef_resource_handler.h"
//#include <platform.h>
//#include <ICryPak.h>
//#include <PMUtils.hpp>

#include <cef_scheme.h>
#include <include/wrapper/cef_stream_resource_handler.h>
#include <Sfinktah/debug.h>
#include <TwSimpleDX11.h>

#ifdef malloc
#define UNDO_UNDEF_ALLOC
#undef malloc
#undef realloc
#undef free
#undef calloc
#endif

#include <string>
#include <Tearless/rapidjson/include/rapidjson/document.h>     // rapidjson's DOM-style API
#include <Tearless/rapidjson/include/rapidjson/prettywriter.h> // for stringify JSON

#ifdef UNDO_DEF_ALLOC
#define malloc   CryModuleCRTMalloc
#define realloc  CryModuleCRTRealloc
#define free     CryModuleCRTFree
#define calloc	 CryModuleCalloc
#undef UNDO_DEF_ALLOC
#endif




using namespace rapidjson;


namespace HTML5Plugin {
	class CEFQueryUrl {
	private:
		std::string jsonReply;
		int status;
	public:
		static std::string ProcessQuery(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback, const char* path, const char* extension, const char* query) {
			CEFQueryUrl q(request, callback, path, extension, query);
			if (!q.status) {
				return std::string("{\"result\";\"FAIL\"}");
			}
			else {
				return q.jsonReply;
			}
		}

		CEFQueryUrl(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback, const char* path, const char* extension, const char* query)
		{
			status = parseJson(query);
		}

		~CEFQueryUrl()
		{
		}

		// Hello World example
		// This example shows basic usage of DOM-style API.


		int parseJson(const char *json) {
			////////////////////////////////////////////////////////////////////////////
			// 1. Parse a JSON text string to a document.

			//const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, \"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";
			printf("Original JSON:\n %s\n", json);

			Document document;  // Default template parameter uses UTF8 and MemoryPoolAllocator.

#if 1
						// "normal" parsing, decode strings to new buffers. Can use other input stream via ParseStream().
			if (document.Parse(json).HasParseError()) {
				printf("\nParsing error\n");
				return 1;
			}
#else
						// In-situ parsing, decode strings directly in the source string. Source must be string.
			char buffer[sizeof(json)];
			memcpy(buffer, json, sizeof(json));
			if (document.ParseInsitu(buffer).HasParseError())
				return 1;
#endif

			printf("\nParsing to document succeeded.\n");

			////////////////////////////////////////////////////////////////////////////
			// 2. Access values in document. 

			printf("\nAccess values in document:\n");
			if (document.IsObject() && document.HasMember("request") && document["request"].IsString()) {
				printf("request found\n");
				if (g_fnJsonCallback) {
					printf("Calling g_fnJsonCallback\n");
					char* json_not_const = _strdup(json);
					jsonReply = g_fnJsonCallback(json_not_const);
					free(json_not_const);
					return 1;
				}
				else {
					printf("Nowhere to send json request\n");
				}
			}
			else {
				printf("request not found");
			}
			assert(document.IsObject());    // Document is a JSON value represents the root of DOM. Root can be either an object or array.

			assert(document.HasMember("request"));
			assert(document["hello"].IsString());
			printf("hello = %s\n", document["hello"].GetString());

			if (document.HasMember("js") && document["js"].IsString()) {
				printf("Executing javascript: %s\n", document["js"].GetString());
				std::string s(document["js"].GetString());
				std::wstring ws;
				int len = s.length();
				for (int i = 0; i < len; i++) {
					ws.append(1, s[i]);
				}
				HTML5Plugin::gPlugin->ExecuteJS(ws.c_str());
			}

			// Since version 0.2, you can use single lookup to check the existing of member and its value:
			Value::MemberIterator hello = document.FindMember("hello");
			assert(hello != document.MemberEnd());
			assert(hello->value.IsString());
			assert(strcmp("world", hello->value.GetString()) == 0);
			(void)hello;

			assert(document["t"].IsBool());     // JSON true/false are bool. Can also uses more specific function IsTrue().
			printf("t = %s\n", document["t"].GetBool() ? "true" : "false");

			assert(document["f"].IsBool());
			printf("f = %s\n", document["f"].GetBool() ? "true" : "false");

			printf("n = %s\n", document["n"].IsNull() ? "null" : "?");

			assert(document["i"].IsNumber());   // Number is a JSON type, but C++ needs more specific type.
			assert(document["i"].IsInt());      // In this case, IsUint()/IsInt64()/IsUInt64() also return true.
			printf("i = %d\n", document["i"].GetInt()); // Alternative (int)document["i"]

			assert(document["pi"].IsNumber());
			assert(document["pi"].IsDouble());
			printf("pi = %g\n", document["pi"].GetDouble());

			{
				const Value& a = document["a"]; // Using a reference for consecutive access is handy and faster.
				assert(a.IsArray());
				for (SizeType i = 0; i < a.Size(); i++) // rapidjson uses SizeType instead of size_t.
					printf("a[%d] = %d\n", i, a[i].GetInt());

				int y = a[0].GetInt();
				(void)y;

				// Iterating array with iterators
				printf("a = ");
				for (Value::ConstValueIterator itr = a.Begin(); itr != a.End(); ++itr)
					printf("%d ", itr->GetInt());
				printf("\n");
			}

			// Iterating object members
			static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };
			for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
				printf("Type of member %s is %s\n", itr->name.GetString(), kTypeNames[itr->value.GetType()]);

			////////////////////////////////////////////////////////////////////////////
			// 3. Modify values in document.

			// Change i to a bigger number
			{
				uint64_t f20 = 1;   // compute factorial of 20
				for (uint64_t j = 1; j <= 20; j++)
					f20 *= j;
				document["i"] = f20;    // Alternate form: document["i"].SetUint64(f20)
				assert(!document["i"].IsInt()); // No longer can be cast as int or uint.
			}

			// Adding values to array.
			{
				Value& a = document["a"];   // This time we uses non-const reference.
				Document::AllocatorType& allocator = document.GetAllocator();
				for (int i = 5; i <= 10; i++)
					a.PushBack(i, allocator);   // May look a bit strange, allocator is needed for potentially realloc. We normally uses the document's.

												// Fluent API
				a.PushBack("Lua", allocator).PushBack("Mio", allocator);
			}

			// Making string values.

			// This version of SetString() just store the pointer to the string.
			// So it is for literal and string that exists within value's life-cycle.
			{
				document["hello"] = "rapidjson";    // This will invoke strlen()
													// Faster version:
													// document["hello"].SetString("rapidjson", 9);
			}

			// This version of SetString() needs an allocator, which means it will allocate a new buffer and copy the the string into the buffer.
			Value author;
			{
				char buffer2[10];
				int len = sprintf(buffer2, "%s %s", "Milo", "Yip");  // synthetic example of dynamically created string.

				author.SetString(buffer2, static_cast<SizeType>(len), document.GetAllocator());
				// Shorter but slower version:
				// document["hello"].SetString(buffer, document.GetAllocator());

				// Constructor version: 
				// Value author(buffer, len, document.GetAllocator());
				// Value author(buffer, document.GetAllocator());
				memset(buffer2, 0, sizeof(buffer2)); // For demonstration purpose.
			}
			// Variable 'buffer' is unusable now but 'author' has already made a copy.
			document.AddMember("author", author, document.GetAllocator());

			assert(author.IsNull());        // Move semantic for assignment. After this variable is assigned as a member, the variable becomes null.

											////////////////////////////////////////////////////////////////////////////
											// 4. Stringify JSON

			printf("\nModified JSON with reformatting:\n");
			StringBuffer sb;
			PrettyWriter<StringBuffer> writer(sb);
			document.Accept(writer);    // Accept() traverses the DOM and generates Handler events.
			puts(sb.GetString());

			return 0;
		}
	};
};

