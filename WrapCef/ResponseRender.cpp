#include "stdafx.h"
#include "ResponseRender.h"
#include <boost/format.hpp>
#include <include/cef_v8.h>
//#include <iostream>
#include <sstream>
//#include "client_app.h"

ResponseRender::ResponseRender()
{
	REGISTER_RESPONSE_FUNCTION(ResponseRender, rsp_invokedJSMethod);
	REGISTER_RESPONSE_FUNCTION(ResponseRender, rsp_callJSMethod);
}


ResponseRender::~ResponseRender()
{
}

template <class T>
std::wstring ConvertToString(T value) {
	std::wstringstream ss;
	ss << value;
	return ss.str();
}

std::wstring V8ValtoWstring(CefRefPtr<CefV8Value> val)
{
	std::wstring strVal;
	if ( val.get() )
	{
		if ( val->IsBool() )
		{
			strVal = val->GetBoolValue() ? L"true" : L"false";
		}
		else if ( val->IsDouble() )
		{
			strVal = ConvertToString(val->GetDoubleValue());
		}
		else if ( val->IsInt() )
		{
			strVal = ConvertToString(val->GetIntValue());
		}
		else if ( val->IsUInt() )
		{
			strVal = ConvertToString(val->GetUIntValue());
		}
		else if ( val->IsNull() )
		{
			strVal = L"null";
		}
		else if ( val->IsString() )
		{
			strVal = val->GetStringValue().ToWString();
		}
		else if ( val->IsUndefined() )
		{
			strVal = L"undefined";
		}
		else{
			strVal = L"undefined";
		}
	}
	return strVal;
}

bool ResponseRender::rsp_invokedJSMethod(const CefRefPtr<CefBrowser> browser, const std::shared_ptr<cyjh::Instruct> req_parm, std::shared_ptr<cyjh::Instruct> outVal)
{
	bool ret = false;
	static std::string _ture("true");
	static std::string _false("false");
	std::string module = req_parm->getList().GetStrVal(0);
	std::string method = req_parm->getList().GetStrVal(1);
	std::string parm = req_parm->getList().GetStrVal(2);
	std::string frameName = req_parm->getList().GetStrVal(3);
	bool bNotifyJson = req_parm->getList().GetBooleanVal(4);
	CefRefPtr<CefFrame> frame;
	if (frameName.empty())
	{
		frame = browser->GetMainFrame();
	}
	else{
		frame = browser->GetFrame(CefString(frameName));
	}
	if ( frame.get() )
	{		
		boost::format fmt("window.invokeMethod('%1%', '%2%', '%3%', %4%)");
		fmt % module % method % parm % (bNotifyJson ? _ture:_false);
		std::string strJs = fmt.str();
		//frame->ExecuteJavaScript(CefString(strJs), CefString(""), 0);
		CefRefPtr<CefV8Context> v8 = frame->GetV8Context();
		CefRefPtr<CefV8Value> retVal;
		CefRefPtr<CefV8Exception> excp;
		CefString cefjs(strJs);
#ifdef _DEBUG
		char szTmp[8192] = { 0 };
		sprintf_s(szTmp, "------js invoke in render  %d ;  %s", GetCurrentThreadId(),
			 strJs.c_str());
		OutputDebugStringA(szTmp);
#endif
		bool bEval = false;
		if (parm.empty())
		{
			bEval = v8->Eval(cefjs, retVal, excp);
		}
		else{
			bEval = v8->CallInvokeMethod(CefString("invokeMethod"), CefString(module), CefString(method), CefString(parm), bNotifyJson, retVal, excp);
		}
		
		if( bEval ){
			if (retVal.get()){
				outVal->getList().AppendVal(V8ValtoWstring(retVal));
			}
			ret = true;
		}else{
			std::string err;
			if ( excp.get() )
			{
				int end = excp->GetEndPosition();
				int col = excp->GetEndColumn();
				int line = excp->GetLineNumber();
				//const char* p = excp->GetScriptResourceName().ToString().c_str();
				//excp->
				int startcol = excp->GetStartColumn();
				//OutputDebugStringA(excp->GetMessageW().ToString().c_str());
				err = excp->GetMessageW().ToString();
			}
#ifdef _DEBUG
			char szTmp[8192] = { 0 };
			sprintf_s(szTmp, "------invokejs Fail! render  %d ; err msg: %s ; %s", GetCurrentThreadId(),
				err.c_str(), strJs.c_str());
			OutputDebugStringA(szTmp);
#endif
		}
	}

#ifdef _DEBUG
	char szTmp[8192] = { 0 };
	sprintf_s(szTmp, "------js invoke okkk!!!  %d ; ", GetCurrentThreadId());
	OutputDebugStringA(szTmp);
#endif
	//assert(ret);
	return ret;
}

bool ResponseRender::rsp_callJSMethod(const CefRefPtr<CefBrowser> browser,
	const std::shared_ptr<cyjh::Instruct> req_parm, std::shared_ptr<cyjh::Instruct> outVal)
{
	bool ret = false;
	std::string funname = req_parm->getList().GetStrVal(0);
	std::string parm = req_parm->getList().GetStrVal(1);
	std::string frameName = req_parm->getList().GetStrVal(2);
	CefRefPtr<CefFrame> frame;
	if (frameName.empty())
	{
		frame = browser->GetMainFrame();
	}
	else{
		frame = browser->GetFrame(CefString(frameName));
	}
	if (frame.get())
	{
		boost::format fmt("%1%('%2%')");
		fmt % funname % parm;
		std::string strJs = fmt.str();
		CefRefPtr<CefV8Context> v8 = frame->GetV8Context();
		CefRefPtr<CefV8Value> retVal;
		CefRefPtr<CefV8Exception> excp;
		if (v8->Eval(CefString(strJs), retVal, excp)){
			if (retVal.get() && retVal.get()->IsString()){
				outVal->getList().AppendVal(retVal->GetStringValue().ToWString());
			}
			ret = true;
		}
	}

	return ret;
}