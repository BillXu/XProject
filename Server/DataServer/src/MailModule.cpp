#include "MailModule.h"
#include "AsyncRequestQuene.h"
#include "log4z.h"
#include "PlayerManager.h"
#include "GameServerApp.h"
#include "PlayerMail.h"
#include "Player.h"
#include <ctime>
void MailModule::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	m_nMaxMailID = 0;
}

void MailModule::onConnectedSvr( bool isReconnected )
{
	if ( isReconnected )
	{
		return;
	}
	auto pSync = getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["sql"] = "SELECT max(mailUID) as maxMailUID FROM mail ;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB,getSvrApp()->getCurSvrIdx(),getSvrApp()->getCurSvrIdx(), eAsync_DB_Select,jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read maxMailUID id error, but no matter ");
			m_nMaxMailID = getSvrApp()->getCurSvrIdx() + getSvrApp()->getCurSvrMaxCnt();
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxMailID = jsRow["maxMailUID"].asUInt() + getSvrApp()->getCurSvrMaxCnt();
		m_nMaxMailID -= m_nMaxMailID % getSvrApp()->getCurSvrMaxCnt();  
		m_nMaxMailID += getSvrApp()->getCurSvrIdx();
#ifdef  _DEBUG
		m_nMaxMailID = 0;
#endif //  _DEBUG
		LOGFMTD("maxMailUID id  = %u", m_nMaxMailID);
	});
}

uint32_t MailModule::generateMailID()
{
	m_nMaxMailID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxMailID;
}

void MailModule::postMail( uint32_t nTargetID, eMailType emailType, Json::Value& jsMailDetail, uint32_t nState)
{
	auto pPlayerMgr = ((DataServerApp*)getSvrApp())->getPlayerMgr();
	auto pPlayer = pPlayerMgr->getPlayerByUserUID( nTargetID );
	auto nNewMailID = generateMailID();
	
	// inform player ;
	if (pPlayer)
	{
		auto pPlayerMail = (CPlayerMailComponent*)pPlayer->getComponent(ePlayerComponent_Mail);
		pPlayerMail->onRecievedMail(nNewMailID, emailType, jsMailDetail, nState, (uint32_t)time(nullptr));
	}

	// save mail to db 
	Json::StyledWriter ss;
	auto jsDetail = ss.write(jsMailDetail);

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "insert into mail ( mailUID,userUID,postTime,mailType,state, mailDetail ) values ( %u,%u,now(),%u,%u,", nNewMailID, nTargetID, emailType, nState);

	std::ostringstream ssSql;
	ssSql << pBuffer << jsDetail << ");";
	jssql["sql"] = ssSql.str();
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, nTargetID, nTargetID, eAsync_DB_Add, jssql);	
}