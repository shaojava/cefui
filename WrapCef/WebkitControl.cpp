#include "stdafx.h"
#include "WebkitControl.h"
#include "client_app.h"
#include "cefclient.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_sandbox_win.h"
#include "include/wrapper/cef_closure_task.h"

// Set focus to |browser| on the UI thread.
static void SetFocusToBrowserControl(CefRefPtr<CefBrowser> browser) {
	if (!CefCurrentlyOn(TID_UI)) {
		// Execute on the UI thread.
		CefPostTask(TID_UI, base::Bind(&SetFocusToBrowserControl, browser));
		return;
	}

	browser->GetHost()->SetFocus(true);
}

////


HWND ChromeiumBrowserControl::AttachHwnd(HWND hParent, const WCHAR* url)
{
	if (!IsWindow(hParent))
	{
		return NULL;
	}
	m_handler = new ClientHandler();
	CefWindowInfo info;
	CefBrowserSettings browser_settings;

	// Populate the browser settings based on command line arguments.
	//AppGetBrowserSettings(browser_settings);
	browser_settings.universal_access_from_file_urls = STATE_ENABLED; //��xpack���ʱ����ļ�

	RECT rect;

	GetClientRect(hParent, &rect);
	info.SetAsChild(hParent, rect);

	CefBrowserHost::CreateBrowser(info, m_handler.get(),
		url, browser_settings, NULL);

	HWND hWnd = NULL;
	if (m_handler->GetBrowser() && m_handler->GetBrowser()->GetHost()){
		hWnd = m_handler->GetBrowser()->GetHost()->GetWindowHandle();
	}
	return hWnd;
}

void ChromeiumBrowserControl::handle_size(HWND hWnd)
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	if (m_handler.get())
	{
		CefWindowHandle hBrowser = m_handler->GetBrowser()->GetHost()->GetWindowHandle();
		HDWP hdwp = BeginDeferWindowPos(1);
		hdwp = DeferWindowPos(hdwp, hBrowser, NULL,
			rect.left, rect.top, rect.right - rect.left,
			rect.bottom - rect.top, SWP_NOZORDER);
		EndDeferWindowPos(hdwp);
	}
}

void ChromeiumBrowserControl::handle_SetForce()
{
	if (m_handler.get())
	{
		CefRefPtr<CefBrowser> browser = m_handler->GetBrowser();
		if (browser)
		{
			SetFocusToBrowserControl(browser);
		}
	}

}

WebkitControl::WebkitControl()
{
	m_ipc_id = 0;
	m_defWinProc = NULL;
	m_browser = new ChromeiumBrowserControl;
}

WebkitControl::~WebkitControl()
{
}

HWND WebkitControl::AttachHwnd(HWND hParentWnd, const WCHAR* url)
{
#if defined _M_AMD64 || defined _WIN64
	m_defWinProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hParentWnd, GWLP_WNDPROC));
	::SetWindowLongPtr(m_MainHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HostWndProc));
	::SetWindowLongPtr(hParentWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
#else
	m_defWinProc = reinterpret_cast<WNDPROC>(::GetWindowLong(hParentWnd, GWL_WNDPROC));
	//�����Զ��崰�ڹ���
	::SetWindowLong(hParentWnd, GWL_WNDPROC, reinterpret_cast<LONG>(HostWndProc));
	::SetWindowLong(hParentWnd, GWL_USERDATA, reinterpret_cast<LONG>(this));
#endif
	return m_browser->AttachHwnd(hParentWnd, url);
}

void WebkitControl::handle_size(HWND hWnd)
{
	m_browser->handle_size(hWnd);
}

void WebkitControl::handle_SetForce()
{
	m_browser->handle_SetForce();
}

LRESULT WebkitControl::HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WebkitControl* control = NULL;
#if defined _M_AMD64 || defined _WIN64
	control = reinterpret_cast<WebkitControl*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
#else
	control = reinterpret_cast<WebkitControl*>(::GetWindowLong(hWnd, GWL_USERDATA));
#endif

	if ( !control )
	{
		return CallWindowProc(DefWindowProc, hWnd, message, wParam, lParam);
	}
	switch (message)
	{
	case WM_SIZE:
	{
		control->handle_size(hWnd);
		return CallWindowProc(control->m_defWinProc, hWnd, message, wParam, lParam);
		break;
	}
	default:
		return CallWindowProc(control->m_defWinProc, hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}