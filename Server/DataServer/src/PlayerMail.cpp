#include "PlayerMail.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "Player.h"
#include <time.h>
#include "PlayerBaseData.h"
#include "DataServerApp.h"
#include "PlayerEvent.h"
#include "AutoBuffer.h"
#include <cassert>
#include "AsyncRequestQuene.h"
CPlayerMailComponent::~CPlayerMailComponent() 
{
	for (auto& ref : m_vMails)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vMails.clear();
}

void CPlayerMailComponent::reset()
{
	for (auto& ref : m_vMails)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vMails.clear();
}

void CPlayerMailComponent::onPlayerLogined()
{
	// read player mail ;
	readMail(0);
}

bool CPlayerMailComponent::onRecievedMail( uint32_t nMailID, eMailType emailType, Json::Value& jsMailDetail, uint32_t& nState, uint32_t nPostTime )
{
	auto iter = m_vMails.find(nMailID);
	if (iter != m_vMails.end())
	{
		LOGFMTE( "already have mailID = %u , uid = %u, duplicat mail ",nMailID ,getPlayer()->getUserUID() );
		return false;
	}

	auto pMail = new stMail();
	pMail->jsDetail = jsMailDetail;
	pMail->nMailID = nMailID;
	pMail->nMailType = emailType;
	pMail->nState = (eMailState)nState;
	pMail->nPostTime = nPostTime;

	m_vMails[pMail->nMailID] = pMail;

	if ( doProcessMail(pMail) )  // check sys processed 
	{
		pMail->nState = eMailState_SysProcessed;
		nState = pMail->nState;  // will save to db in mail module::postMail; ;
	}

	Json::Value jsTellClient;
	jsTellClient["mailID"] = nMailID;
	jsTellClient["type"] = emailType;
	jsTellClient["state"] = nState;
	jsTellClient["detail"] = jsMailDetail;
	jsTellClient["time"] = nPostTime;
	sendMsg(jsTellClient, MSG_NEW_MAIL);

	informMaxMailID();
	return true;
}

bool CPlayerMailComponent::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)
{
	switch (nmsgType)
	{
	case MSG_PLAYER_REQ_MAILS:
	{
		uint32_t nClientMaxMailID = recvValue["clientMaxMailID"].asUInt();
#ifdef _DEBUG
		nClientMaxMailID = 0;
#endif // _DEBUG

		uint8_t nPage = 0;
		Json::Value jsArrayM;
		for (auto& ref : m_vMails)
		{
			if ( ref.first <= nClientMaxMailID)
			{
				continue;
			}

			Json::Value jsMail;
			jsMail["mailID"] = ref.second->nMailID;
			jsMail["type"] = ref.second->nMailType;
			jsMail["state"] = ref.second->nState;
			jsMail["detail"] = ref.second->jsDetail;
			jsMail["time"] = ref.second->nPostTime;
			jsArrayM[jsArrayM.size()] = jsMail;
			if (jsArrayM.size() == 10 )
			{
				Json::Value jsmsg;
				jsmsg["pageIdx"] = nPage++;
				jsmsg["mails"] = jsArrayM;
				sendMsg(jsmsg, nmsgType);
				jsArrayM.clear();
			}
		}

		Json::Value jsmsg;
		jsmsg["pageIdx"] = nPage;
		jsmsg["mails"] = jsArrayM;
		sendMsg(jsmsg, nmsgType);
	}
	break;
	case MSG_PLAYER_PROCESS_MAIL:
	{
		// client : { mailID : 23 , state : eMailState, arg : {}  } . arg : can be null , depend on mail type and process type ;
		uint32_t nMailID = recvValue["mailID"].asUInt();
		uint32_t state = recvValue["state"].asUInt();
		auto iterMail = m_vMails.find(nMailID);
		if (iterMail == m_vMails.end())
		{
			recvValue["ret"] = 1;
			sendMsg(recvValue, nmsgType);
			break;
		}

		if ( iterMail->second->nState == state )
		{
			recvValue["ret"] = 2;
			sendMsg(recvValue, nmsgType);
			break;
		}
		recvValue["ret"] = 0;
		sendMsg(recvValue, nmsgType);
		iterMail->second->nState = (eMailState)state;
		std::vector<uint32_t> v;
		v.push_back(nMailID);
		updateMailsStateToDB(v, iterMail->second->nState);
		if ( eMailState_Delete == state )
		{
			m_vMails.erase(iterMail);
		}
	}
	break;
	default:
		return false ;
	}
	return true;
}

void CPlayerMailComponent::onPlayerOtherDeviceLogin(uint32_t nOldSessionID, uint32_t nNewSessionID)
{
	informMaxMailID();
}

bool CPlayerMailComponent::doProcessMail(stMail* pMail)
{
	if (pMail == nullptr || eMailState_WaitSysAct != pMail->nState )
	{
		return false;
	}

	// do process mail act 
	switch ( pMail->nMailType )
	{
	case eMail_AppleStore_Pay:
	case eMail_Wechat_Pay:
	{
		if ( getPlayer()->getBaseData()->isPlayerReady() && pMail->jsDetail["ret"].asUInt() == 0 )
		{
			auto nDiamondCnt = pMail->jsDetail["diamondCnt"].asUInt();
			getPlayer()->getBaseData()->modifyMoney((int32_t)nDiamondCnt, true );
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_Agent_AddCard:
	{
		if (getPlayer()->getBaseData()->isPlayerReady())
		{
			auto nDiamondCnt = pMail->jsDetail["cardOffset"].asInt();
			getPlayer()->getBaseData()->modifyMoney((int32_t)nDiamondCnt, true);
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_Consume_Diamond:
	{
		if (getPlayer()->getBaseData()->isPlayerReady())
		{
			auto nDiamondCnt = pMail->jsDetail["diamond"].asUInt();
			getPlayer()->getBaseData()->modifyMoney((int32_t)nDiamondCnt * -1 , true);
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_Consume_Emoji:
	{
		if (getPlayer()->getBaseData()->isPlayerReady())
		{
			auto nDiamondCnt = pMail->jsDetail["cnt"].asUInt();
			getPlayer()->getBaseData()->modifyEmojiCnt((int32_t)nDiamondCnt * -1 );
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_Agent_AddEmojiCnt:
	{
		if (getPlayer()->getBaseData()->isPlayerReady())
		{
			auto nDiamondCnt = pMail->jsDetail["addCnt"].asInt();
			getPlayer()->getBaseData()->modifyEmojiCnt((int32_t)nDiamondCnt);
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_GiveBack_Diamond:
	{
		if (getPlayer()->getBaseData()->isPlayerReady())
		{
			auto nDiamondCnt = pMail->jsDetail["diamond"].asUInt();
			getPlayer()->getBaseData()->modifyMoney((int32_t)nDiamondCnt, true);
		}
		else
		{
			return false;
		}
	}
	break;
	case eMail_ClubJoin:
	{
		uint32_t nClubID = pMail->jsDetail["clubID"].asUInt();
		getPlayer()->getBaseData()->onJoinClub( nClubID );
	}
	break;
	case eMail_ClubLeave:
	{
		uint32_t nClubID = pMail->jsDetail["clubID"].asUInt();
		getPlayer()->getBaseData()->onLeaveClub( nClubID );
	}
	break;
	default:
		return false;
	}
	pMail->nState = eMailState_SysProcessed;
	return true;
}

void CPlayerMailComponent::readMail( uint16_t nOffset )
{
	auto asyq = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	std::ostringstream ss;
	ss << "SELECT mailUID,unix_timestamp(postTime) as postTime ,mailType,mailDetail,state FROM mail where userUID = " << getPlayer()->getUserUID() << " and state !=  " << eMailState_Delete << " order by mailUID desc limit 3 offset " << nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB,getPlayer()->getUserUID(),eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		if (isTimeOut)
		{
			doProcessMailAfterReadDB();
			return;
		}

		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			doProcessMailAfterReadDB();
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx )
		{
			auto jsRow = jsData[nRowIdx];
			auto nMailID = jsRow["mailUID"].asUInt();
			auto iter = m_vMails.find(nMailID);
			if (iter != m_vMails.end())
			{
				LOGFMTE("why already have this mail id = %u , uid = %u double read , bug",nMailID,getPlayer()->getUserUID() );
				doProcessMailAfterReadDB();
				return;
			}

			Json::Value jsDetail;
			Json::Reader jsReader;
			jsReader.parse( jsRow["mailDetail"].asString(), jsDetail, false);

			auto pMail = new stMail();
			pMail->jsDetail = jsDetail;
			pMail->nMailID = nMailID;
			pMail->nMailType = (eMailType)jsRow["mailType"].asUInt();
			pMail->nState = (eMailState)jsRow["state"].asUInt();
			pMail->nPostTime = jsRow["postTime"].asUInt();

			m_vMails[pMail->nMailID] = pMail;
		}

		if ( nAft < 3 )  // only read one page ;
		{
			doProcessMailAfterReadDB();
			return;
		}

		// not finish , go on read 
		auto nSize = m_vMails.size();
		if (nSize >= 10 )
		{
			doProcessMailAfterReadDB();
			return;
		}

		readMail(nSize);;

	});
}

void CPlayerMailComponent::doProcessMailAfterReadDB()
{
	std::vector<uint32_t> vUpdateStateMails;
	for ( auto& iter : m_vMails )
	{
		if (doProcessMail(iter.second))
		{
			vUpdateStateMails.push_back(iter.first);
		}
	}

	if ( vUpdateStateMails.size() > 0 )
	{
		updateMailsStateToDB(vUpdateStateMails,eMailState_SysProcessed);
	}

	informMaxMailID();
}

void CPlayerMailComponent::updateMailsStateToDB(std::vector<uint32_t>& vMailIDs, eMailState eNewState)
{
	std::ostringstream ss;
	ss << "UPDATE mail SET state = " << eNewState << " where mailUID = " << vMailIDs[0]; 
	for ( uint16_t nIdx = 1; nIdx < vMailIDs.size(); ++nIdx)
	{
		ss << " and mailUID = " << vMailIDs[nIdx];
	}
	ss << " limit " << vMailIDs.size() << ";";

	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	auto asyq = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jsReq);
}

void CPlayerMailComponent::informMaxMailID()
{
	uint32_t nMaxmailID = 0;
	if (m_vMails.empty() == false)
	{
		nMaxmailID = m_vMails.rbegin()->first;
	}

	Json::Value js;
	js["maxMailID"] = nMaxmailID;
	sendMsg(js, MSG_PLAYER_RECIEVED_NEW_MAIL);
}