#include "MailModule.h"
#include "AsyncRequestQuene.h"
#include "log4z.h"
#include "PlayerManager.h"
#include "DataServerApp.h"
#include "PlayerMail.h"
#include "Player.h"
#include <ctime>
#include "PlayerBaseData.h"
void MailModule::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	m_nMaxMailID = 0;
}

void MailModule::onConnectedSvr( bool isReconnected )
{
	//return;
	if ( isReconnected )
	{
		return;
	}
	auto pSync = getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["sql"] = "SELECT max(mailUID) as maxMailUID FROM mail ;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB,getSvrApp()->getCurSvrIdx(),eAsync_DB_Select,jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		if ( isTimeOut )
		{
			return ;
		}
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
		//m_nMaxMailID = 0;
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
	if ( pPlayer && pPlayer->isPlayerReady() )
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
	ssSql << pBuffer << " ' "<< jsDetail << " ' );";
	auto s = ssSql.str();
	jssql["sql"] = ssSql.str();
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, nTargetID, eAsync_DB_Add, jssql);	

	// save diamond log
	if (eMail_Owner_Pay == emailType || eMail_Agent_AddCard == emailType || eMail_Consume_Diamond == emailType || eMail_GiveBack_Diamond == emailType || ( (eMail_Wechat_Pay == emailType || eMail_AppleStore_Pay == emailType || eMail_Consume_Golden_Emoji == emailType) && jsMailDetail["ret"].asUInt() == 0 ) )
	{
		int32_t nOffset = 0;
		uint8_t nLogDiamond = eLogDiamond_Max;
		if (eMail_Consume_Diamond == emailType || eMail_GiveBack_Diamond == emailType)
		{
			nOffset = jsMailDetail["diamond"].asInt() * (eMail_Consume_Diamond == emailType ? -1 : 1 );
			nLogDiamond = eLogDiamond_Room;
		}
		else if (eMail_Agent_AddCard == emailType)
		{
			nOffset = jsMailDetail["cardOffset"].asInt();
			nLogDiamond = eLogDiamond_Agent;
		}
		else if ( eMail_Wechat_Pay == emailType || eMail_AppleStore_Pay == emailType)
		{
			nOffset = jsMailDetail["diamondCnt"].asInt();
			if (eMail_Wechat_Pay == emailType) {
				nLogDiamond = eLogDiamond_Shop_Wechat;
			}
			else if(eMail_AppleStore_Pay == emailType) {
				nLogDiamond = eLogDiamond_Shop_AppStore;
			}
			/*else {
				nLogDiamond = eLogDiamond_Shop_Owner;
			}*/
			//nLogDiamond = eMail_Wechat_Pay == emailType ? eLogDiamond_Shop_Wechat : eLogDiamond_Shop_AppStore;
		}
		else if (eMail_Owner_Pay == emailType) {
			nOffset = jsMailDetail["amount"].asInt();
			nLogDiamond = eLogDiamond_Shop_Owner;
		}
		else if (eMail_Consume_Golden_Emoji == emailType) {
			nOffset = jsMailDetail["amount"].asInt();
			nLogDiamond = eLogCoin_Room;
		}

		uint32_t nFinal = 0;
		if (pPlayer)
		{
			if (eMail_Owner_Pay == emailType || eMail_Consume_Golden_Emoji == emailType) {
				nFinal = pPlayer->getBaseData()->getCoin();
			}
			else {
				nFinal = pPlayer->getBaseData()->getDiamoned();
			}
		}

		CPlayer::saveDiamondRecorder(nTargetID, nLogDiamond, nOffset, nFinal, jsMailDetail);
	}
}