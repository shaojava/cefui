// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// This file is shared by cefclient and cef_unittests so don't include using
// a qualified path.
#include "stdafx.h"
#include "client_app.h"  // NOLINT(build/include)

#include <string>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "include/wrapper/cef_helpers.h"
#include "BrowserIdentifier.h"
#include "IPC.h"

ClientApp* ClientApp::s_app = NULL;

ClientApp::ClientApp() {
	s_app = this;
	UIThreadSync_ = new cyjh::UIThreadCombin;
	RenderThreadSync_ = new cyjh::RenderThreadCombin;
}

void ClientApp::OnRegisterCustomSchemes(
    CefRefPtr<CefSchemeRegistrar> registrar) {
  // Default schemes that support cookies.
  cookieable_schemes_.push_back("http");
  cookieable_schemes_.push_back("https");

  RegisterCustomSchemes(registrar, cookieable_schemes_);
}

void ClientApp::OnContextInitialized() {
  CreateBrowserDelegates(browser_delegates_);

  // Register cookieable schemes with the global cookie manager.
  CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager();
  DCHECK(manager.get());
  manager->SetSupportedSchemes(cookieable_schemes_);

  print_handler_ = CreatePrintHandler();

  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnContextInitialized(this);
}

void ClientApp::OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnBeforeChildProcessLaunch(this, command_line);
}

std::wstring RandChr(int len)
{
	const WCHAR CCH[] = L"abcdefghijklmnopqrstuvwxyz";

	static unsigned int stal = 0;
	++stal;
	unsigned int processid = GetProcessId(GetCurrentProcess());
	srand((unsigned)time(NULL) + stal + processid);

	WCHAR* ch = new WCHAR[len + 1];
	memset(ch, 0, sizeof(WCHAR)*(len + 1));

	for (int i = 0; i < len; ++i)
	{
		int x = rand() % (wcslen(CCH) - 1);

		ch[i] = CCH[x];
	}
	static std::wstring strRand;
	strRand.clear();
	strRand.append(ch);
	delete[] ch;

	return strRand;
}

void ClientApp::OnRenderProcessThreadCreated(
    CefRefPtr<CefListValue> extra_info) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  static int num = 0;
  ++num;
  WCHAR szSrvPipeName[64] = { 0 };
  WCHAR szCliPipeName[64] = { 0 };
  swprintf_s(szSrvPipeName, L"\\\\.\\pipe\\srv_%s_%d", RandChr(3).c_str(), num);
  swprintf_s(szCliPipeName, L"\\\\.\\pipe\\cli_%s_%d", RandChr(3).c_str(), num);
  extra_info->SetString(extra_info->GetSize(), CefString(szSrvPipeName)); //主线程设置扩展属性
  extra_info->SetString(extra_info->GetSize(), CefString(szCliPipeName));
  //OutputDebugString(L"OnRenderProcessThreadCreated");
  std::shared_ptr<cyjh::IPCUnit> spIpc = cyjh::IPC_Manager::getInstance().GenerateIPC(szSrvPipeName, szCliPipeName);
  spIpc.get()->BindRecvCallback(&cyjh::UIThreadCombin::RecvData, UIThreadSync_.get());
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnRenderProcessThreadCreated(this, extra_info);
}

void ClientApp::OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) {
  CreateRenderDelegates(render_delegates_);

  RenderDelegateSet::iterator it = render_delegates_.begin();
  CefString srvPipe = extra_info->GetString(extra_info->GetSize() - 2);
  CefString cliPipe = extra_info->GetString(extra_info->GetSize() - 1); //子线程得到扩展属性
  //OutputDebugString(L"OnRenderThreadCreated");
  std::shared_ptr<cyjh::IPCUnit> spIpc = cyjh::IPC_Manager::getInstance().GenerateIPC(cliPipe.ToWString().c_str(), srvPipe.ToWString().c_str());
  spIpc.get()->BindRecvCallback(&cyjh::RenderThreadCombin::RecvData, RenderThreadSync_.get());
  //OutputDebugString(L"OnRenderThreadCreated222222222222");
  RenderThreadSync_->SetIpc(spIpc);
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnRenderThreadCreated(this, extra_info);
}

void ClientApp::OnWebKitInitialized() {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
	DWORD Id = browser->GetIdentifier();
	BrowserIdentifier::GetInst().InsertBrowser(Id, browser);
	/*{
		//测试代码
		cyjh::Instruct parm;
		parm.setName("sfse");		
		std::shared_ptr<cyjh::Instruct> outval;
		RenderThreadSync_.Request(browser, parm, outval);
	}*/
	//OutputDebugString(L"OnBrowserCreated++++");
	cyjh::Instruct parm;
	parm.setName("RegisterBrowser");
	parm.setInstructType(cyjh::InstructType::INSTRUCT_REGBROWSER);
	parm.getList().AppendVal(std::wstring(RenderThreadSync_->getIpc().get()->getCliName()));
	parm.getList().AppendVal(std::wstring(RenderThreadSync_->getIpc().get()->getSrvName()));
	std::shared_ptr<cyjh::Instruct> outval;
	RenderThreadSync_->Request(browser, parm, outval);
	//OutputDebugString(L"OnBrowserCreated2222222");
  RenderDelegateSet::iterator it = render_delegates_.begin();  
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserCreated(this, browser);
}

void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
	DWORD Id = browser->GetIdentifier();
	BrowserIdentifier::GetInst().RemoveBrowser(Id);
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserDestroyed(this, browser);
}

CefRefPtr<CefLoadHandler> ClientApp::GetLoadHandler() {
  CefRefPtr<CefLoadHandler> load_handler;
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end() && !load_handler.get(); ++it)
    load_handler = (*it)->GetLoadHandler(this);

  return load_handler;
}

bool ClientApp::OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefRequest> request,
                                   NavigationType navigation_type,
                                   bool is_redirect) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    if ((*it)->OnBeforeNavigation(this, browser, frame, request,
                                  navigation_type, is_redirect)) {
      return true;
    }
  }

  return false;
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextReleased(this, browser, frame, context);
}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientApp::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefDOMNode> node) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnFocusedNodeChanged(this, browser, frame, node);
}

bool ClientApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  bool handled = false;

  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }

  return handled;
}

bool ClientApp::OnProcessMessageReceived2(CefRefPtr<CefBrowser> browser,
	CefProcessId source_process,
	CefRefPtr<CefProcessMessage> message, CefRefPtr<CefListValue> response, bool& response_ack){
	DCHECK_EQ(source_process, PID_BROWSER);
	bool handled = false;

	RenderDelegateSet::iterator it = render_delegates_.begin();
	for (; it != render_delegates_.end() && !handled; ++it) {
		handled = (*it)->OnProcessMessageReceived2(this, browser, source_process,
			message, response, response_ack);
	}

	return handled;
}

bool ClientApp::OnProcessResponseReceived(CefRefPtr<CefBrowser> browser,
	CefProcessId source_process, int request_id,
	bool succ,
	CefRefPtr<CefListValue> response){

	bool handled = false;
	RenderDelegateSet::iterator it = render_delegates_.begin();
	for (; it != render_delegates_.end() && !handled; ++it) {
		handled = (*it)->OnProcessResponseReceived(this, browser, source_process, request_id, succ, response);
	}
	return handled;
}

bool ClientApp::OnProcessResponseAckReceived(CefRefPtr<CefBrowser> browser,
	CefProcessId source_process, int request_id){

	bool handled = false;
	RenderDelegateSet::iterator it = render_delegates_.begin();
	for (; it != render_delegates_.end() && !handled; ++it) {
		handled = (*it)->OnProcessResponseAckReceived(this, browser, source_process,
			request_id);
	}
	return handled;
}

/*
//add by lincoln
void ClientApp::RenderDelegate::OnContextCreated(CefRefPtr<ClientApp> app,
	CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	CefRefPtr<CefV8Context> context)
{
	class Accessor : public CefV8Accessor {
	public:
		Accessor() : val_(CefV8Value::CreateBool(true)) {}
		virtual bool Get(const CefString& name,
			const CefRefPtr<CefV8Value> object,
			CefRefPtr<CefV8Value>& retval,
			CefString& exception) OVERRIDE{
			retval = val_;
			return true;
		}
			virtual bool Set(const CefString& name,
			const CefRefPtr<CefV8Value> object,
			const CefRefPtr<CefV8Value> value,
			CefString& exception) OVERRIDE{
			return true;
		}
		CefRefPtr<CefV8Value> val_;
		IMPLEMENT_REFCOUNTING(Accessor);
	};
	CefRefPtr<CefV8Value> window = context->GetGlobal();
	CefRefPtr<CefV8Accessor> myV8Acc = new Accessor;
	CefRefPtr<CefV8Value> val = CefV8Value::CreateString(L"Application");
	CefString cefException;
	myV8Acc->Set(L"name", window, val, cefException);
	CefRefPtr<CefV8Value> pObjApp = CefV8Value::CreateObject(myV8Acc);
	window->SetValue(L"Application", pObjApp, V8_PROPERTY_ATTRIBUTE_NONE);
	const char myfuname[] = "myfoo";
	CefRefPtr<CefV8Handler> myV8handle = new Handler();
	CefRefPtr<CefV8Value> myFun = CefV8Value::CreateFunction(myfuname, myV8handle);
	CefV8Value::PropertyAttribute attributes =
		static_cast<CefV8Value::PropertyAttribute>(
		V8_PROPERTY_ATTRIBUTE_READONLY |
		V8_PROPERTY_ATTRIBUTE_DONTENUM |
		V8_PROPERTY_ATTRIBUTE_DONTDELETE);

	DCHECK_EQ(context->GetGlobal()->SetValue(myfuname, myFun, attributes), false);
	//context->Exit();
}*/
//end by licnoln