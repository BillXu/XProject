#include "PlayerMail.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "Player.h"
#include <time.h>
#include "PlayerBaseData.h"
#include "GameServerApp.h"
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
	return true;
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
			auto nDiamondCnt = pMail->jsDetail["cardOffset"].asUInt();
			getPlayer()->getBaseData()->modifyMoney((int32_t)nDiamondCnt, true);
		}
		else
		{
			return false;
		}
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
	ss << "SELECT mailUID,unix_timestamp(postTime) as postTime ,mailType,mailDetail,state FROM mail where userUID = " << getPlayer()->getUserUID() << "and state !=  " << eMailState_Delete << " order by mailUID desc limit 3 offset " << nOffset << ";";
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
			LOGFMTE("read max bill id error ");
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
}

void CPlayerMailComponent::updateMailsStateToDB(std::vector<uint32_t>& vMailIDs, eMailState eNewState)
{
	std::ostringstream ss;
	ss << "UPDATE mail SET state = " << eNewState << "where mailUID = " << vMailIDs[0]; 
	for ( uint16_t nIdx = 1; nIdx < vMailIDs.size(); ++nIdx)
	{
		ss << "and mailUID = " << vMailIDs[nIdx];
	}
	ss << " limit " << vMailIDs.size() << ";";

	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	auto asyq = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jsReq);
}