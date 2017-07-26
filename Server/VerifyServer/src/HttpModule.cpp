#include "tinyxml/tinyxml.h"
#include "Md5.h"
#include "HttpModule.h"
#include "log4z.h"
#include "VerifyApp.h"
#include "ConfigDefine.h"
#include "json/json.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "TaskPoolModule.h"
#include "VerifyApp.h"
#include "AnyLoginTask.h"
#include "VerifyApp.h"
void CHttpModule::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	
	std::string strNotifyUrl = ((CVerifyApp*)svrApp)->getWebchatNotifyUrl();
	// parse port ;
	std::size_t nPosDot = strNotifyUrl.find_last_of(':');
	std::size_t nPosSlash = strNotifyUrl.find_last_of('/');
	uint16_t nPort = 80;
	if (nPosDot != std::string::npos && std::string::npos != nPosSlash)
	{
		auto strPort = strNotifyUrl.substr(nPosDot + 1 , nPosSlash - nPosDot - 1 );
		nPort = atoi(strPort.c_str());
		if (0 == nPort)
		{
			nPort = 80;
		}
	}
	//
	mHttpServer = std::make_shared<http::server::server>(nPort);
	mHttpServer->run();

	// parse uri 
	std::size_t nPos = strNotifyUrl.find_last_of('/');
	std::string strUri = strNotifyUrl.substr(nPos,strNotifyUrl.size() - nPos );
	registerHttpHandle( strUri, std::bind(&CHttpModule::onHandleVXPayResult, this, std::placeholders::_1));

	registerHttpHandle("/playerInfo.yh", std::bind(&CHttpModule::handleGetPlayerInfo, this, std::placeholders::_1));
	registerHttpHandle("/addRoomCard.yh", std::bind(&CHttpModule::handleAddRoomCard, this, std::placeholders::_1));
	registerHttpHandle("/AnyLogin.yh", std::bind(&CHttpModule::handleAnySdkLogin, this, std::placeholders::_1));
}

void CHttpModule::update(float fDeta)
{
	IGlobalModule::update(fDeta);
	std::set<http::server::connection_ptr> vOut;
	if (!mHttpServer->getRequestConnects(vOut))
	{
		return;
	}

	for (auto& ref : vOut)
	{
		auto req = ref->getReqPtr();
		auto pReply = ref->getReplyPtr();
		auto iter = vHttphandles.find( req->uri );
		if (iter == vHttphandles.end())
		{
			LOGFMTE("no handle for uri = %s",req->uri.c_str());
			*pReply = http::server::reply::stock_reply(http::server::reply::bad_request);
			ref->doReply();
			continue;
		}
		
		auto pfunc = iter->second;
		if ( !pfunc(ref) )
		{
			*pReply = http::server::reply::stock_reply(http::server::reply::bad_request);
			ref->doReply();
		}
	}
	vOut.clear();
}

bool CHttpModule::registerHttpHandle(std::string strURI, httpHandle pHandle)
{
	auto iter = vHttphandles.find(strURI);
	if (iter != vHttphandles.end())
	{
		LOGFMTE("already register handle for uri = %s",strURI.c_str());
		return false;
	}
	vHttphandles[strURI] = pHandle;
	return true;
}

 std::string getXmlNodeValue(const char* pnodeName, TiXmlNode* pRoot)
{
	std::string str = "";
	TiXmlElement* pValueParent = (TiXmlElement*)pRoot->FirstChild(pnodeName);
	if (pValueParent)
	{
		TiXmlNode* pValue = pValueParent->FirstChild();
		if (pValue)
		{
			str = pValue->Value();
			return str;
		}
	}
	LOGFMTE("xml node = %s value is null", pnodeName);
	return str;
}

// process vx pay result 
bool CHttpModule::onHandleVXPayResult(http::server::connection_ptr ptr)
{
	auto req = ptr->getReqPtr();
	auto res = ptr->getReplyPtr();

	if (req->contentSize == 0)
	{
		return false;
	}

	// parse xml 
	TiXmlDocument t;
	t.Parse(req->reqContent.c_str(), 0, TIXML_ENCODING_UTF8);
	if (t.Error())
	{
		LOGFMTE("vx pay parse xml error : %s",t.ErrorDesc());
		return false;
	}
#ifndef Node_Value_F  
#define node_Value( x ) getXmlNodeValue((x),pRoot)
#define Node_Value_F ;
#endif 
	TiXmlNode* pRoot = t.RootElement();
	uint8_t nRet = 0;
	do
	{
		if (!pRoot)
		{
			nRet = 1;
			break;
		}

		auto retCode = node_Value("return_code");
		if (retCode != "SUCCESS")
		{
			nRet = 3;
			auto strEmsg = node_Value("return_msg");
			LOGFMTE("ret msg : %s", strEmsg.c_str());
			break;
		}
 
		// do parse 
		auto fee = node_Value("total_fee");
		auto strTradeNo = node_Value("out_trade_no");
		auto payResult = node_Value("result_code");
		auto payTime = node_Value("time_end");
		if ("FAIL" == payResult)
		{
			return true ;
		}
		
		std::vector<std::string> vOut;
		StringSplit(strTradeNo,"E",vOut);
		if (vOut.size() < 2)
		{
			LOGFMTE("trade out error = %s",strTradeNo.c_str() );
			nRet = 4;
			break;
		}
		auto shopItem = vOut[0];
		auto userUID = vOut[1];
		LOGFMTD( "GO TO DB Verify trade = %s , fee = %s payTime = %s",strTradeNo.c_str(),fee.c_str(),payTime.c_str() );

		// do DB verify ;
		auto pVeirfyModule = ((CVerifyApp*)getSvrApp())->getTaskPoolModule();
		pVeirfyModule->doDBVerify(atoi(userUID.c_str()), atoi(shopItem.c_str()), ePay_WeChat,strTradeNo,atoi(fee.c_str()));
	} while (0);

	std::string str = "";
	if (nRet != 0)
	{
		str = "<xml><return_code><![CDATA[FAIL]]></return_code> <return_msg><![CDATA[unknown]]></return_msg> </xml> ";
	}
	else
	{
		str = "<xml><return_code><![CDATA[SUCCESS]]></return_code> <return_msg><![CDATA[OK]]></return_msg> </xml> ";
	}

	res->setContent(str,"text/xml");
	ptr->doReply();
	t.SaveFile("reT.xml");
	return true;
}

bool CHttpModule::handleGetPlayerInfo(http::server::connection_ptr ptr)
{
	auto req = ptr->getReqPtr();
	auto res = ptr->getReplyPtr();
	LOGFMTD("reciveget player info req = %s",req->reqContent.c_str());

	Json::Reader jsReader;
	Json::Value jsRoot;
	auto bRet = jsReader.parse(req->reqContent, jsRoot);
	if (!bRet)
	{
		LOGFMTE("parse agent get player info argument error");
		return false;
	}

	uint32_t nUID = 0, nAgentID = 0;
	if (jsRoot["playerUID"].isNull() || jsRoot["playerUID"].isUInt() == false )
	{
		LOGFMTD("cant not finn uid argument");
		return false;
	}

	if (jsRoot["agentID"].isNull() || jsRoot["agentID"].isUInt() == false)
	{
		LOGFMTD("cant not find agentID argument");
		return false;
	}

	nUID = jsRoot["playerUID"].asUInt();
	nAgentID = jsRoot["agentID"].asUInt();
	// do async request 
	Json::Value jsReq;
	jsReq["targetUID"] = nUID;
	jsReq["agentID"] = nAgentID;
	auto async = getSvrApp()->getAsynReqQueue();
	async->pushAsyncRequest(ID_MSG_PORT_DATA,nUID,eAsync_AgentGetPlayerInfo, jsReq, [ptr, nUID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
	{
		// build msg to send ;
		Json::Value jsRespone;
		jsRespone["ret"] = retContent["ret"];
		jsRespone["name"] = retContent["name"];
		jsRespone["playerUID"] = nUID;
		jsRespone["cardCnt"] = retContent["leftCardCnt"];
		jsRespone["isOnline"] = retContent["isOnline"];

		Json::StyledWriter jswrite;
		auto str = jswrite.write(jsRespone);
		auto res = ptr->getReplyPtr();
		res->setContent(str, "text/json");
		ptr->doReply();
	});
	LOGFMTD("do async agent get player info uid = %u", nUID);
	return true;
}

bool CHttpModule::handleAddRoomCard(http::server::connection_ptr ptr)
{
	auto req = ptr->getReqPtr();
	auto res = ptr->getReplyPtr();
	LOGFMTD("reciveget add room card info req = %s", req->reqContent.c_str());

	Json::Reader jsReader;
	Json::Value jsRoot;
	auto bRet = jsReader.parse(req->reqContent, jsRoot);
	if (!bRet)
	{
		LOGFMTE("parse add room card argument error");
		return false;
	}

	uint32_t nUID = 0, nAgentID = 0 ;
	uint32_t nAddCard;
	uint32_t nAddCardNo;
	if (jsRoot["playerUID"].isNull() || jsRoot["playerUID"].isUInt() == false )
	{
		LOGFMTD("cant not finn uid argument");
		return false;
	}

	if (jsRoot["addCard"].isNull() || jsRoot["addCard"].isInt() == false)
	{
		LOGFMTD("cant not finn addCard argument");
		return false;
	}

	if (jsRoot["addCardNo"].isNull() || jsRoot["addCardNo"].isUInt() == false)
	{
		LOGFMTD("cant not finn addCardNo argument");
		return false;
	}

	if (jsRoot["agentID"].isNull() || jsRoot["agentID"].isInt() == false)
	{
		LOGFMTD("cant not finn agentID argument");
		return false;
	}

	nAddCard = jsRoot["addCard"].asInt();
	nUID = jsRoot["playerUID"].asUInt();
	nAgentID = jsRoot["agentID"].asUInt();
	nAddCardNo = jsRoot["addCardNo"].asUInt();
	// do async request 
	Json::Value jsReq;
	jsReq["targetUID"] = nUID;
	jsReq["addCard"] = nAddCard;
	jsReq["addCardNo"] = nAddCardNo;
	jsReq["agentID"] = nAgentID;
	auto async =  getSvrApp()->getAsynReqQueue();
	async->pushAsyncRequest(ID_MSG_PORT_DATA, nUID, eAsync_AgentAddRoomCard, jsReq, [ptr, nUID, nAddCardNo](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
	{
		auto res = ptr->getReplyPtr();
		// do check 
		jsUserData["ret"] = 1;
		Json::StyledWriter jswrite;
		auto str = jswrite.write(jsUserData);
		res->setContent(str, "text/json");
		ptr->doReply();
		LOGFMTD("do agent add room cards uid = %u, addCardNo = %u ", nUID, nAddCardNo);
	}, jsRoot);
	LOGFMTD("do async agent add room cards uid = %u cnt = %d,addCardNo = %u", nUID, nAddCard, nAddCardNo);
	return true;
}

bool CHttpModule::handleAnySdkLogin(http::server::connection_ptr ptr)
{
	auto req = ptr->getReqPtr();
	auto res = ptr->getReplyPtr();
	LOGFMTD("reciveget any sdk req = %s", req->reqContent.c_str());
	auto pTaskPool = ((CVerifyApp*)getSvrApp())->getTaskPoolModule();
	auto pTask = pTaskPool->getReuseTask(CTaskPoolModule::eTask_AnyLogin);
	auto pLoginTask = (AnyLoginTask*)pTask.get();
	pLoginTask->setReqString(req->reqContent);
	pTask->setCallBack([ptr](ITask::ITaskPrt pTask)
	{
		auto pLoginTask = (AnyLoginTask*)pTask.get();
		auto res = ptr->getReplyPtr();
		if (pTask->getResultCode() == 1)
		{
			LOGFMTE("request error from any sdk");
			// do check 
			std::string str = "error";
			res->setContent(str, "text/json");
			ptr->doReply();
			return;
		}

		auto jsResult = pLoginTask->getResultJson();
		Json::StyledWriter jswrite;
		auto str = jswrite.write(jsResult);
		res->setContent(str, "text/json");
		ptr->doReply();
		LOGFMTE("any sdk resp: %s",str.c_str());
	});
	pTaskPool->postTask(pTask);
	return true;
}
