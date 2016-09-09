#include <StdAfx.h>
#include <windowsx.h>
#include "../../../../../AuthorityProjectConfig.h"
#include <Tearless/CEF/include/wrapper/cef_helpers.h>
#include <Tearless/CEF/cefclient/browser/geometry_util.h>
#include <CPluginHTML5.h>
//#include <windowsx.h>
//#include <cefclient/browser/geometry_util.h>
//#include <cefclient/browser/main_message_loop.h>
//#include <cefclient/browser/resource.h>
//#include <cefclient/browser/util_win.h>
//#include <include/cef_app.h>


namespace HTML5Plugin {
	/// Tearless WndProc, returns 1 if processing should continue, 0 if event process should stop.
	LRESULT CALLBACK CWndProc::OsrWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		//CEF_REQUIRE_UI_THREAD();
		//gPlugin->LogError("CWndProc::OsrWndProc: CefCurrentlyOn() = %i", CefCurrentlyOn(TID_UI));

		// CWndProc* self = GetUserDataPtr<CWndProc*>(hWnd);
		CWndProc* self = HTML5Plugin::gWndProc;

		if (!self)
			return 1;
		//return DefWindowProc(hWnd, message, wParam, lParam);

		switch (message) {
		case WM_LBUTTONDOWN:
			::OutputDebugString(TEXT("Left Mouse Button click received by CWndProc OsrWndProc"));
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
			//  self->OnPaint();
			//  return 0;

			//case WM_ERASEBKGND:
			//  if (self->OnEraseBkgnd())
			//      break;
			//  // Don't erase the background.
			//  return 0;

			//case WM_NCDESTROY:
			//  // Clear the reference to |self|.
			//  SetUserDataPtr(hWnd, NULL);
			//  self->hwnd_ = NULL;
			//  break;
		}

		return 1;
		//return DefWindowProc(hWnd, message, wParam, lParam);
	}

	void CWndProc::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam) {
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

		switch (message) {
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
			}
			else {
				CefBrowserHost::MouseButtonType btnType =
					(message == WM_LBUTTONDOWN ? MBT_LEFT : (
						message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
				if (!cancelPreviousClick && (btnType == last_click_button_)) {
					++last_click_count_;
				}
				else {
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
				//              //renderer_.SetSpin(0, 0);
				//Invalidate();
			}
			else {
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
					//  IsOverPopupWidget(x, y) &&
					//  (GetPopupXOffset() || GetPopupYOffset())) {
					//  break;
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
				//              renderer_.IncrementSpin(
				//current_mouse_pos_.x - last_mouse_pos_.x,
				//current_mouse_pos_.y - last_mouse_pos_.y);
				last_mouse_pos_.x = current_mouse_pos_.x;
				last_mouse_pos_.y = current_mouse_pos_.y;
				//Invalidate();
			}
			else {
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
				POINT screen_point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
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
					isKeyDown(VK_SHIFT) ? delta : 0,
					!isKeyDown(VK_SHIFT) ? delta : 0);
			}
			break;
		}
	}


	void CWndProc::OnSizing(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		LPRECT rect = (RECT *)(lParam);
		LONG width;
		LONG widthIdeal;
		LONG height;
		FLOAT ratio;
		hwnd_ = hwnd;
		width = rect->right - rect->left;
		height = rect->bottom - rect->top;
		::GetClientRect(hwnd_, &client_rect_);
		SetDeviceScaleFactor(
			(client_rect_.right - client_rect_.left) / TEARLESS_WINDOW_WIDTH,
			(client_rect_.bottom - client_rect_.top) / TEARLESS_WINDOW_HEIGHT
		);
	}

	//void CWndProc::OnSize() {
	//  // Keep |client_rect_| up to date.
	//  ::GetClientRect(hwnd_, &client_rect_);

	//  SetDeviceScaleFactor(
	//      (client_rect_.right - client_rect_.left) / TEARLESS_WINDOW_WIDTH,
	//      (client_rect_.bottom - client_rect_.top) / TEARLESS_WINDOW_HEIGHT
	//  );
	//  if (browser_)
	//      browser_->GetHost()->WasResized();
	//}

	void CWndProc::OnSize() {
		// Keep |client_rect_| up to date.
		::GetClientRect(hwnd_, &client_rect_);

		if (browser_)
			browser_->GetHost()->WasResized();
	}



	void CWndProc::OnFocus(bool setFocus) {
		if (browser_)
			browser_->GetHost()->SendFocusEvent(setFocus);
	}

	void CWndProc::OnCaptureLost() {
		if (mouse_rotation_)
			return;

		if (browser_)
			browser_->GetHost()->SendCaptureLostEvent();
	}

	void CWndProc::OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {
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

	void CWndProc::OnPaint() {
		//// Paint nothing here. Invalidate will cause OnPaint to be called for the
		//// render handler.
		//PAINTSTRUCT ps;
		//BeginPaint(hwnd_, &ps);
		//EndPaint(hwnd_, &ps);

		//if (browser_)
		//  browser_->GetHost()->invalidate(PET_VIEW);
	}

	bool CWndProc::OnEraseBkgnd() {
		// Erase the background when the browser does not exist.
		return (browser_ == NULL);
	}

#ifdef TEARLESS_HAS_RENDERER
	bool CWndProc::IsOverPopupWidget(int x, int y) const {
		CEF_REQUIRE_UI_THREAD();
		const CefRect& rc = renderer_.popup_rect();
		int popup_right = rc.x + rc.width;
		int popup_bottom = rc.y + rc.height;
		return (x >= rc.x) && (x < popup_right) &&
			(y >= rc.y) && (y < popup_bottom);
	}

	int CWndProc::GetPopupXOffset() const {
		CEF_REQUIRE_UI_THREAD();
		return renderer_.original_popup_rect().x - renderer_.popup_rect().x;
	}

	int CWndProc::GetPopupYOffset() const {
		CEF_REQUIRE_UI_THREAD();
		return renderer_.original_popup_rect().y - renderer_.popup_rect().y;
	}

	void CWndProc::ApplyPopupOffset(int& x, int& y) const {
		if (IsOverPopupWidget(x, y)) {
			x += GetPopupXOffset();
			y += GetPopupYOffset();
		}
	}
#endif

//	void CWndProc::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
//		DEBUG_OUT("CWndProc::OnAfterCreated");
//		CEF_REQUIRE_UI_THREAD();
//		DCHECK(!browser_);
//		browser_ = browser;
//
//		if (hwnd_) {
//			// Show the browser window. Called asynchronously so that the browser has
//			// time to create associated internal objects.
//			CefPostTask(TID_UI, base::Bind(&CWndProc::Show, this));
//		}
//#ifdef TEARLESS_MESSAGE_ROUTER
//		AnotherMessageRouter();
//		CreateMessageRouter();
//#endif
//	}

	// This is copied from CEF offscren demo cefclient_osr_widget_win section
	bool CWndProc::isKeyDown(WPARAM wparam) {
		return (GetKeyState(wparam) & 0x8000) != 0;
	}

	int CWndProc::GetCefMouseModifiers(WPARAM wparam) {
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

	int CWndProc::GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
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

	//bool CWndProc::IsOverPopupWidget(int x, int y) const {
	//  //  const CefRect& rc = renderer_.popup_rect();
	//  int popup_right = rc.x + rc.width;
	//  int popup_bottom = rc.y + rc.height;
	//  return (x >= rc.x) && (x < popup_right) &&
	//      (y >= rc.y) && (y < popup_bottom);
	//}

	//int CWndProc::GetPopupXOffset() const {
	//  //  return renderer_.original_popup_rect().x - renderer_.popup_rect().x;
	//}

	//int CWndProc::GetPopupYOffset() const {
	//  //  return renderer_.original_popup_rect().y - renderer_.popup_rect().y;
	//}

	//void CWndProc::ApplyPopupOffset(int& x, int& y) const {
	//  if (IsOverPopupWidget(x, y)) {
	//      x += GetPopupXOffset();
	//      y += GetPopupYOffset();
	//  }
	//}


	// end copy from section

	// This is copied from CEF offscreen demo, uncomment or comment as needed

	//void CWndProc::CreateBrowser(HWND parent_hwnd,
	//  const RECT& rect,
	//  CefRefPtr<CefClient> handler,
	//  const CefBrowserSettings& settings,
	//  CefRefPtr<CefRequestContext> request_context,
	//  const std::string& startup_url) {
	//  if (!CefCurrentlyOn(TID_UI)) {
	//      // Execute this method on the UI thread.
	//      CefPostTask(TID_UI, base::Bind(&CWndProc::CreateBrowser, this,
	//          parent_hwnd, rect, handler, settings,
	//          request_context, startup_url));
	//      return;
	//  }

	//  // Create the native window.
	//  Create(parent_hwnd, rect);

	//  CefWindowInfo window_info;
	//  //  window_info.SetAsWindowless(hwnd_, renderer_.IsTransparent());
	//  // settings.background_color = 0xffffff80;
	//  // Create the browser asynchronously.
	//  CefBrowserHost::CreateBrowser(window_info, handler, startup_url, settings,
	//      request_context);
	//}

#ifdef TEARLESS_HANDLES_POPUPS
	void CWndProc::ShowPopup(HWND parent_hwnd,
		int x, int y, size_t width, size_t height) {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CWndProc::ShowPopup, this,
				parent_hwnd, x, y, width, height));
			return;
		}

		DCHECK(browser_.get());

		// Create the native window.
		const RECT rect = { x, y,
			x + static_cast<int>(width),
			y + static_cast<int>(height) };
		//Create(parent_hwnd, rect);

		// Send resize notification so the compositor is assigned the correct
		// viewport size and begins rendering.
		browser_->GetHost()->WasResized();

		Show();
	}

	void CWndProc::Show() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CWndProc::Show, this));
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

	void CWndProc::Hide() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CWndProc::Hide, this));
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

	void CWndProc::SetBounds(int x, int y, size_t width, size_t height) {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CWndProc::SetBounds, this, x, y, width,
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

	void CWndProc::SetFocus() {
		if (!CefCurrentlyOn(TID_UI)) {
			// Execute this method on the UI thread.
			CefPostTask(TID_UI, base::Bind(&CWndProc::SetFocus, this));
			return;
		}

		if (hwnd_) {
			// Give focus to the native window.
			::SetFocus(hwnd_);
		}
	}
#endif

	void CWndProc::SetDeviceScaleFactor(float device_scale_factor_width, float device_scale_factor_height) {
		//if (!CefCurrentlyOn(TID_UI)) {
		//  // Execute this method on the UI thread.
		//  CefPostTask(TID_UI, base::Bind(&CWndProc::SetDeviceScaleFactor, this,
		//      device_scale_factor));
		//  return;
		//}

		if (device_scale_factor_width_ == device_scale_factor_width
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
}

