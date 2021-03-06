#include "stdafx.h"
#include "NormalWebFactory.h"

NormalWebFactory NormalWebFactory::s_inst;

NormalWebFactory::NormalWebFactory()
{
	
}


NormalWebFactory::~NormalWebFactory()
{
}

void NormalWebFactory::CreateNewWebControl(const HWND& hwnd, const WCHAR* url, const WCHAR* cookie_ctx, const bool skipcache)
{
	CefRefPtr<WebkitControl> item = new  WebkitControl;
	m_map.insert(std::make_pair(hwnd, item));
	item->AttachHwnd(hwnd, url, cookie_ctx, skipcache);	
}

void NormalWebFactory::CreateNewWebControl(const HWND& hwnd, CefRefPtr<ClientHandler> clientHandle)
{
	CefRefPtr<WebkitControl> item = new  WebkitControl;
	if ( item->getBrowser().get() )
	{
		item->getBrowser()->setClientHandler(clientHandle);
	}
	m_map.insert(std::make_pair(hwnd, item));
	item->SubWindow_Proc(hwnd);
}

bool NormalWebFactory::CloseWebControl(const HWND& hwnd)
{
	bool ret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if ( it != m_map.end() )
	{
		if (it->second->getBrowser().get() )
		{
			if (it->second->getBrowser()->close())
			{
				m_map.erase(it);
				ret = true;
			}
		}		
	}
	return ret;
}

CefRefPtr<CefBrowser> NormalWebFactory::GetBrowser(int browserID)
{
	CefRefPtr<CefBrowser> browser;
	NormalWebMap::iterator it = m_map.begin();
	for (; it != m_map.end(); ++it)
	{
		CefRefPtr<CefBrowser> visit = it->second->getBrowser()->getBrowser();
		if ( visit && visit->GetIdentifier() == browserID )
		{
			browser = visit;
			break;
		}
	}
	return browser;
}

CefRefPtr<WebkitControl> NormalWebFactory::GetWebkitControlByID(int browserID)
{
	CefRefPtr<WebkitControl> control;
	NormalWebMap::iterator it = m_map.begin();
	for (; it != m_map.end(); ++it)
	{
		CefRefPtr<CefBrowser> visit = it->second->getBrowser()->getBrowser();
		if (visit && visit->GetIdentifier() == browserID)
		{
			control = it->second;
			break;
		}
	}
	return control;
}

CefRefPtr<WebkitControl> NormalWebFactory::GetWebkitControlByHostHwnd(HWND hWnd)
{
	CefRefPtr<WebkitControl> control;
	NormalWebMap::iterator it = m_map.find(hWnd);
	if ( it != m_map.end() )
	{
		control = it->second;
	}
	return control;
}

bool NormalWebFactory::SetBrowser(HWND hWnd, CefRefPtr<CefBrowser> browser)
{
	bool ret = false;
	CefRefPtr<WebkitControl> control = GetWebkitControlByHostHwnd(hWnd);
	if (control.get() && control->getBrowser().get() )
	{
		ret = control->getBrowser()->setBrowser(browser);
	}
	return ret;
}

void NormalWebFactory::CloseAll()
{
	NormalWebMap::iterator it = m_map.begin();
	for (; it != m_map.end(); ++it)
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->close();
		}
	}
}

bool NormalWebFactory::Loadurl(const HWND& hwnd, const WCHAR* url)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if ( it->second->getBrowser().get() )
		{
			it->second->getBrowser()->loadUrl(url);
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::GoBack(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->back();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::GoForward(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->forward();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::Reload(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->reload();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::ReloadIgnoreCache(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->reloadIgnoreCache();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::IsAudioMuted(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->IsAudioMuted();
			bret = true;
		}
	}
	return bret;
}

void NormalWebFactory::SetAudioMuted(const HWND& hwnd, const bool& bEnable)
{
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->SetAudioMuted(bEnable);
		}
	}
}

bool NormalWebFactory::Stop(const HWND& hwnd)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->Stop();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::asyncInvokedJSMethod(const HWND& hwnd, const char* utf8_module, const char* utf8_method,
	const char* utf8_parm,
	const char* utf8_frame_name, bool bNoticeJSTrans2JSON)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			bret = it->second->getBrowser()->asyncInvokedJSMethod(utf8_module,
				utf8_method, utf8_parm,
				utf8_frame_name, 
				bNoticeJSTrans2JSON);
		}
	}
	return bret;
}

void NormalWebFactory::AdjustRenderSpeed(const HWND& hWnd, const double& dbSpeed)
{
	NormalWebMap::iterator it = m_map.find(hWnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->AdjustRenderSpeed(dbSpeed);
		}
	}
}

void NormalWebFactory::SendMouseClickEvent(const HWND& hWnd, const unsigned int& msg, const long& wp, const long& lp)
{
	NormalWebMap::iterator it = m_map.find(hWnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->SendMouseClickEvent(msg, wp, lp);
		}
	}
}

bool NormalWebFactory::InjectJS(const HWND& hwnd, const WCHAR* js)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get())
		{
			it->second->getBrowser()->InjectJS(js);
		}
		bret = true;
	}
	return bret;
}

bool NormalWebFactory::QueryRenderProcessID(const HWND& hwnd, int& pid)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get() && it->second->getBrowser()->getBrowser().get()
			&& it->second->getBrowser()->getBrowser()->GetHost().get() )
		{
			pid = it->second->getBrowser()->getBrowser()->GetHost()->GetRenderSystemProcessID();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::QueryPluginsProcessID(const HWND& hwnd, std::vector<DWORD>& plugins_process_ids)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get() && it->second->getBrowser()->getBrowser().get()
			&& it->second->getBrowser()->getBrowser()->GetHost().get())
		{
			int render_process_id = it->second->getBrowser()->getBrowser()->GetHost()->GetRenderProcessID();
			fn_GetRenderPluginProcessIDs fun = (fn_GetRenderPluginProcessIDs)GetProcAddress(GetModuleHandle(L"libwbx.dll"), "GetRenderPluginProcessIDs");
			if ( fun )
			{
				fun(render_process_id, plugins_process_ids);
				bret = true;
			}
		}
	}
	return bret;
}

bool NormalWebFactory::GetViewZoomLevel(const HWND& hwnd, double& level)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get() && it->second->getBrowser()->getBrowser().get()
			&& it->second->getBrowser()->getBrowser()->GetHost().get())
		{
			level = it->second->getBrowser()->getBrowser()->GetHost()->GetZoomLevel();
			bret = true;
		}
	}
	return bret;
}

bool NormalWebFactory::SetViewZoomLevel(const HWND& hwnd, const double& level)
{
	bool bret = false;
	NormalWebMap::iterator it = m_map.find(hwnd);
	if (it != m_map.end())
	{
		if (it->second->getBrowser().get() && it->second->getBrowser()->getBrowser().get()
			&& it->second->getBrowser()->getBrowser()->GetHost().get())
		{
			it->second->getBrowser()->getBrowser()->GetHost()->SetZoomLevel(level);
			bret = true;
		}
	}
	return bret;
}