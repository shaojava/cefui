#include "stdafx.h"
#include "WebViewFactory.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_sandbox_win.h"
#include "include/wrapper/cef_closure_task.h"

#include "client_app.h"
#include "cefclient.h"
#include "NormalWebFactory.h"

WebViewFactory WebViewFactory::s_inst;

WebViewFactory::WebViewFactory()
{
}


WebViewFactory::~WebViewFactory()
{
}

// Set focus to |browser| on the UI thread.
static void SetFocusBrowser(CefRefPtr<CefBrowser> browser) {
	if (!CefCurrentlyOn(TID_UI)) {
		// Execute on the UI thread.
		CefPostTask(TID_UI, base::Bind(&SetFocusBrowser, browser));
		return;
	}

	browser->GetHost()->SetFocus(true);
}

HWND WebViewFactory::GetWebView(const HWND& hSameProcessWnd, const HINSTANCE& hInstance, const int& x, const int& y, const int& width,
	const int& height, const CefString& url, const int& alpha, const bool& taskbar, const bool& trans, const int& sizetype)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
#ifdef _DEBUG
	char szTmp[8192] = { 0 };
	sprintf_s(szTmp, "------GetWebView   %s", url.ToString().c_str() );
	OutputDebugStringA(szTmp);
#endif
	CefRefPtr<CefBrowser> browser;
	{
		if (hSameProcessWnd && IsWindow(hSameProcessWnd))
		{
			CefRefPtr<WebItem> item = FindItem(hSameProcessWnd);
			if ( item.get() )
			{
				browser = item->getBrowserByHwnd(hSameProcessWnd);
			}
		}
	}

	CefRefPtr<WebItem> item = new  WebItem;
	item->m_handle = new ClientHandler();
	item->m_handle->SetMainWindowHandle(NULL);
	CefRefPtr<BrowserProvider> provider = new BrowserProvider(item->m_handle);
	CefWindowInfo info;
	CefBrowserSettings browser_settings;
	const bool transparent = false;
	//cmd_line->HasSwitch(cefclient::kTransparentPaintingEnabled);
	const bool show_update_rect = false;
	RECT rect;

	CefRefPtr<OSRWindow> window =
		OSRWindow::Create(provider, transparent,
		show_update_rect);
	WCHAR szOSRWindowClass[] = L"WebViewWindowClass";
	rect.left = x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom = y + height;
	window->CreateWidget(NULL, rect, hInstance, szOSRWindowClass, trans, sizetype);
	assert(IsWindow(window->hwnd()));
	bool bTest = item->m_window_map.insert(std::make_pair(window->hwnd(), window)).second;
	assert(bTest == true);

	info.SetAsWindowless(window->hwnd(), transparent);
	info.transparent_painting_enabled = trans; //是否需要?
	info.windowless_rendering_enabled = true;
	//item->m_handle->SetOSRHandler(window.get());
	if ( !taskbar )
	{
		DWORD exStyle = GetWindowLong(window->hwnd(), GWL_EXSTYLE);
		exStyle &= ~WS_EX_APPWINDOW;
		exStyle |= WS_EX_TOOLWINDOW;
		SetWindowLong(window->hwnd(), GWL_EXSTYLE, exStyle);
		SetWindowPos(window->hwnd(), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	window->SetAlpha(alpha);
	//ShowWindow(item->m_window->hwnd(), SW_SHOW);

	//browser_settings.file_access_from_file_urls = STATE_ENABLED;
	browser_settings.universal_access_from_file_urls = STATE_ENABLED; //让xpack访问本地文件
	//browser_settings.web_security = STATE_DISABLED; //网页安全
	//browser_settings.webgl = STATE_DISABLED;
	//browser_settings.plugins = STATE_DISABLED;
	//browser_settings.java = STATE_DISABLED;
	//browser_settings.application_cache = STATE_DISABLED; //不用缓存
	// Creat the new child browser window

	//CefRequestContextSettings settings;
	//CefString(&settings.cache_path) = CefString(g_strGlobalCachePath.c_str());
	/*if (!shared_request_context_.get()) {
		shared_request_context_ =
			CefRequestContext::CreateContext(CefRequestContext::GetGlobalContext(),
			new ClientRequestContextHandler);
	}*/

	/*if ( m_requestContextHandler.get() == NULL )
	{
		m_requestContextHandler = new RequestContextHandlerPath(g_strGlobalCachePath.c_str());
	}
	CefRefPtr<CefRequestContext> request_context = CefRequestContext::CreateContext(settings, m_requestContextHandler);*/


	m_viewList.push_back(item);
	if ( browser.get() )
	{
		CefBrowserHost::CreateInheritBrowser(browser, info, item->m_handle,
			url, browser_settings, CookiesManage::getInst().getShareRequest() );
	}
	else{
		CefBrowserHost::CreateBrowser(info, item->m_handle,
			url, browser_settings, CookiesManage::getInst().getShareRequest());
	}
	//WCHAR szBuf[] = { L"D:\\work\\WebUIDemo\\bin\\Release\\uiframe\\PepperFlash1\\pepflashplayer.dll;application/x-shockwave-flash" };
	//item->m_provider->GetBrowser()->RegPlugin(szBuf, true);
	return window->hwnd();
}

void WebViewFactory::RemoveWindow(HWND hWnd)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		if ( (*it)->removeHwnd(hWnd) )
		{
			if ((*it)->size() == 0){
				m_viewList.erase(it);
			}
			break;
		}
	}
}

CefRefPtr<WebItem> WebViewFactory::FindItem(const HWND& hWnd)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	CefRefPtr<WebItem> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		if ((*it)->isHit(hWnd)){
			ptr = *it;
			break;
		}
	}
	return ptr;
}

CefRefPtr<CefBrowser> WebViewFactory::GetBrowser(int browserID)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	CefRefPtr<CefBrowser> ptr_ret;
	WebViewList::iterator it = m_viewList.begin();
	bool bFind = false;
	for (; it != m_viewList.end(); ++it)
	{
		CefRefPtr<CefBrowser> ptr = (*it)->getBrowser(browserID);
		if (ptr.get())
		{
			ptr_ret = ptr;
			bFind = true;
			break;
		}
	}
	return ptr_ret;
}

CefRefPtr<WebItem> WebViewFactory::GetBrowserItem(int browserID)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	CefRefPtr<WebItem> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		if ( (*it)->isHitItem(browserID) )
		{
			ptr = (*it);
			break;
		}
	}
	return ptr;
}

HWND WebViewFactory::GetBrowserHwndByID(int browserID)
{
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	HWND hWnd = NULL;
	CefRefPtr<CefBrowser> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		CefRefPtr<CefBrowser>browser = (*it)->getBrowser(browserID);
		if (browser.get())
		{
			hWnd = browser->GetHost()->GetWindowHandle();
			break;
		}
	}

	return hWnd;
}

void WebViewFactory::CloseAll()
{
	NormalWebFactory::getInstance().CloseAll();
	//std::unique_lock<std::mutex> lock(factoryMutex_);
	std::vector<CefRefPtr<WebItem>> weblist;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		//it->second->m_provider->GetBrowser()->GetHost()->CloseBrowser(true);
		//it = m_viewMap.erase(it);
		weblist.push_back( *it );
	}

	std::vector<CefRefPtr<WebItem>>::iterator webit = weblist.begin();
	for ( ; webit != weblist.end(); ++webit )
	{
		(*webit)->closeAll();
	}
}

void WebViewFactory::ClearData(int compType)
{
	if (!CefCurrentlyOn(TID_UI)){
		CefPostTask(TID_UI, base::Bind(&WebViewFactory::ClearData, this, compType));
		return;
	}

	int len = m_viewList.size();
	if ( len > 0 )
	{
		WebViewList::iterator it = m_viewList.begin();
		//it->second->m_provider->GetBrowser()->GetHost()->CleaarData(compType);
		(*it)->clearData(compType);
	}

}

bool WebViewFactory::InjectJS(const HWND& hwnd, const WCHAR* js)
{
	bool bret = false;
	CefRefPtr<WebItem> item = FindItem(hwnd);
	if ( item.get() && item->m_handle.get() )
	{
		item->m_handle->initiativeInjectJS(item->m_handle->GetBrowser(), js);
		bret = true;
	}
	return bret;
}

bool WebViewFactory::QueryRenderProcessID(const HWND& hwnd, int& pid)
{
	bool bret = false;
	CefRefPtr<WebItem> item = FindItem(hwnd);
	if (item.get() && item->m_handle.get() && item->m_handle->GetBrowser().get()
		&& item->m_handle->GetBrowser()->GetHost().get())
	{
		CefRefPtr<CefBrowser>browser = item->m_handle->GetBrowser();
		pid = browser->GetHost()->GetRenderSystemProcessID();
		bret = true;
	}
	return bret;
}

bool WebViewFactory::QueryPluginsProcessID(const HWND& hwnd, std::vector<DWORD>& plugins_process_ids)
{
	bool bret = false;
	CefRefPtr<WebItem> item = FindItem(hwnd);
	if (item.get() && item->m_handle.get() && item->m_handle->GetBrowser().get()
		&& item->m_handle->GetBrowser()->GetHost().get() )
	{
		CefRefPtr<CefBrowser>browser = item->m_handle->GetBrowser();
		int render_process_id = browser->GetHost()->GetRenderProcessID();
		fn_GetRenderPluginProcessIDs fun = (fn_GetRenderPluginProcessIDs)GetProcAddress(GetModuleHandle(L"libwbx.dll"), "GetRenderPluginProcessIDs");
		if ( fun )
		{
			fun(render_process_id, plugins_process_ids);
			bret = true;
		}
	}
	return bret;
}

bool WebViewFactory::GetViewZoomLevel(const HWND& hwnd, double& level)
{
	bool bret = false;
	CefRefPtr<WebItem> item = FindItem(hwnd);
	if (item.get() && item->m_handle.get() && item->m_handle->GetBrowser().get()
		&& item->m_handle->GetBrowser()->GetHost().get())
	{
		level = item->m_handle->GetBrowser()->GetHost()->GetZoomLevel();
		bret = true;
	}
	return bret;
}

bool WebViewFactory::SetViewZoomLevel(const HWND& hwnd, const double& level)
{
	bool bret = false;
	CefRefPtr<WebItem> item = FindItem(hwnd);
	if (item.get() && item->m_handle.get() && item->m_handle->GetBrowser().get()
		&& item->m_handle->GetBrowser()->GetHost().get())
	{
		item->m_handle->GetBrowser()->GetHost()->SetZoomLevel(level);
		bret = true;
	}
	return bret;
}

CefRefPtr<CefBrowser> WebViewFactory::GetBrowserByHwnd(const HWND& hWnd)
{
	CefRefPtr<CefBrowser> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		ptr = (*it)->getBrowserByHwnd(hWnd);
		if ( ptr.get() )
		{
			break;
		}
	}
	return ptr;
}

CefRefPtr<OSRWindow> WebViewFactory::getWindowByHwnd(const HWND& hWnd)
{
	CefRefPtr<OSRWindow> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		ptr = (*it)->getWindowByHwnd(hWnd);
		if ( ptr.get() )
		{
			break;
		}
	}
	return ptr;
}

CefRefPtr<OSRWindow> WebViewFactory::getWindowByID(const int& browserID)
{
	CefRefPtr<OSRWindow> ptr;
	WebViewList::iterator it = m_viewList.begin();
	for (; it != m_viewList.end(); ++it)
	{
		ptr = (*it)->getWindowByID(browserID);
		if (ptr.get())
		{
			break;
		}
	}
	return ptr;
}