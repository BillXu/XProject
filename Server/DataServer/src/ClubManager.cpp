#include "ClubManager.h"
#include "Club.h"
#include "log4z.h"
#include "DataServerApp.h"
#include "Player.h"
#include "AsyncRequestQuene.h"
#include "PlayerBaseData.h"
ClubManager::~ClubManager()
{
	for (auto& ref : m_vClubs)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vClubs.clear();
}

bool ClubManager::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID )
{
	if ( nMsgType > MSG_CLUB_MSG_END || nMsgType < MSG_CLUB_MSG)
	{
		return false;
	}

	if ( MSG_CLUB_CHECK_NAME == nMsgType)
	{
		uint8_t nRet = 0;
		if ( isNameDuplicate( prealMsg["name"].asString() ) )
		{
			nRet = 1;
		}
		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
		return true;
	}

	if ( MSG_CLUB_CREATE_CLUB == nMsgType )
	{
		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
		uint8_t nRet = 0;
		if (pPlayer == nullptr || pPlayer->getSessionID() != nSenderID )
		{
			nRet = 4;
		}
		else if (canPlayerCreateClub(pPlayer->getUserUID()) == false)
		{
			nRet = 1;
		}
		else if (isNameDuplicate(prealMsg["name"].asString()))
		{
			nRet = 2;
		}

		if ( nRet )
		{
			Json::Value js;
			js["ret"] = nRet;
			sendMsg( js, nMsgType,nTargetID,nSenderID,ID_MSG_PORT_CLIENT);
			return true;
		}

		auto p = new Club();
		p->init(this,generateClubID(), prealMsg["name"].asString(), prealMsg["opts"], 1000, 8 );
		p->addMember(pPlayer->getUserUID(), eClubPrivilige_Creator);
		m_vClubs[p->getClubID()] = p;

		Json::Value js;
		js["ret"] = nRet;
		js["clubID"] = p->getClubID();
		sendMsg(js, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);

		// save to db 
		Json::StyledWriter jsw;
		auto strOpts = jsw.write(prealMsg["opts"]);
		Json::Value jssql;
		char pBuffer[2048] = { 0 };
		sprintf_s(pBuffer,sizeof(pBuffer),"insert into clubs ( clubID,ownerUID,name,opts ) values ( %u,%u,'%s','%s');", p->getClubID(), p->getCreatorUID(), prealMsg["name"].asCString(), strOpts.c_str() );
		jssql["sql"] = pBuffer;
		getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100 ,eAsync_DB_Add, jssql);

		if ( prealMsg["opts"].isNull() )
		{
			LOGFMTE( "argumet erros opts is null " );
			return true;
		}

		// inform hei zi 
		Json::Value jsPost;
		jsPost["url"] = "http://nn365.youhoox.com?ct=club&&ac=add";
		Json::Value jsPostData;
		jsPostData["ct"] = "club";
		jsPostData["ac"] = "add" ;
		jsPostData["mobile"] = prealMsg["opts"]["phoneNum"];
		jsPostData["club_id"] = p->getClubID();
		jsPostData["pay_type"] = prealMsg["opts"]["payType"];
		jsPost["postData"] = jsPostData;
		getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_VERIFY, rand() % 100, eAsync_HttpPost, jsPost);
		p->onFirstCreated();
		return true;
	}
	 

	uint32_t nClubID = 0;
	if (prealMsg["clubID"].isNull() || prealMsg["clubID"].isUInt() == false)
	{
		LOGFMTE( "club msg = %u , do not have clubID key ",nMsgType );
		prealMsg["ret"] = 201;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
		return true;
	}

	nClubID = prealMsg["clubID"].asUInt();
	if ( MSG_CLUB_DISMISS_CLUB == nMsgType)
	{
		uint8_t nRet = 0;
		Club* pClub = nullptr;
		uint32_t nUserID = 0;
		do 
		{
			auto iter = m_vClubs.find( nClubID );
			if ( iter == m_vClubs.end())
			{
				nRet = 200;
				break;
			}
			pClub = iter->second;

			if ( !pClub->canDismiss() )
			{
				nRet = 3;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr || pPlayer->getSessionID() != nSenderID)
			{
				nRet = 4;
				break;
			}
			nUserID = pPlayer->getUserUID();
			if ( pClub->getCreatorUID() != pPlayer->getUserUID() )
			{
				nRet = 1;
				break;
			}

		} while ( 0 );

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);

		if ( nRet )
		{
			return true;
		}

		pClub->onWillDismiss();
		
		auto iter = m_vClubs.find(nClubID);
		delete iter->second;
		iter->second = nullptr;
		m_vClubs.erase(iter);
		LOGFMTD( "club id = %u dissmissed uid = %u", nClubID,nUserID);


		// save to db 
		Json::StyledWriter jsw;
		auto strOpts = jsw.write(prealMsg["opts"]);
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf(pBuffer, "update clubs set isDelete = 1 where clubID = %u;", nClubID );
		jssql["sql"] = pBuffer;
		getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Update, jssql);

		return true;
	}

	auto iter = m_vClubs.find( nClubID );
	if (iter == m_vClubs.end())
	{
		LOGFMTE("club msg = %u , clubID is null ", nMsgType);
		prealMsg["ret"] = 200;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
		return true;
	}

	auto pclub = iter->second;
	return pclub->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID, nTargetID);
}

bool ClubManager::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint32_t nTargetID) {
	if (eAsync_HttpCmd_CreateClub == nRequestType) {
		uint8_t nRet = 0;
		if (jsReqContent["targetUID"].isNull()) {
			nRet = 1;
		}

		if (isNameDuplicate(jsReqContent["name"].asString())) {
			nRet = 2;
		}

		if (nRet) {
			Json::Value jsAgentBack;
			jsAgentBack["ret"] = nRet;
			getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
			return true;
		}

		auto nUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nUID);
		if (pPlayer == nullptr) {
			Json::Value jssql;
			char pBuffer[512] = { 0 };
			sprintf_s(pBuffer, "SELECT userUID FROM playerbasedata where userUID = %u ;", nUID);
			std::string str = pBuffer;
			jssql["sql"] = pBuffer;
			getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, nUID, eAsync_DB_Select, jssql, [this, nSenderPort, nReqSerial, nSenderID, nUID, jsReqContent](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					Json::Value jsAgentBack;
					jsAgentBack["ret"] = 7;
					getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
					return;
				}
				Json::Value jsData = retContent["data"];
				if (jsData.size() == 0)
				{
					Json::Value jsAgentBack;
					jsAgentBack["ret"] = 3;
					getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
					LOGFMTE("why read player uid = %u create club is null ? ", nUID);
					return;
				}
				
				auto pPlayerManager = DataServerApp::getInstance()->getPlayerMgr();
				pPlayerManager->doPlayerLogin(nUID);
				auto pPlayer = pPlayerManager->getPlayerByUserUID(nUID);
				if (pPlayer == nullptr) {
					Json::Value jsAgentBack;
					jsAgentBack["ret"] = 4;
					getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
					LOGFMTE("why read player uid = %u do not have this player create club ? ", nUID);
					return;
				}

				auto pClub = new Club();
				auto jsCreateOpts = jsReqContent["opts"];
				auto sCreateName = jsReqContent["name"].asString();
				pClub->init(this, generateClubID(), sCreateName, jsCreateOpts, 1000, 8);
				pClub->addMember(pPlayer->getUserUID(), eClubPrivilige_Creator);
				m_vClubs[pClub->getClubID()] = pClub;
				pPlayer->getBaseData()->onCreatedClub(pClub->getClubID());

				// save to db 
				Json::StyledWriter jsw;
				auto strOpts = jsw.write(jsCreateOpts);
				Json::Value jssql;
				char pBuffer[2048] = { 0 };
				sprintf_s(pBuffer, sizeof(pBuffer), "insert into clubs ( clubID,ownerUID,name,opts ) values ( %u,%u,'%s','%s');", pClub->getClubID(), pClub->getCreatorUID(), sCreateName.c_str(), strOpts.c_str());
				jssql["sql"] = pBuffer;
				getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Add, jssql);
				LOGFMTE("player uid = %u create club = %u ? ", nUID, pClub->getClubID());

				Json::Value jsAgentBack;
				jsAgentBack["ret"] = 0;
				jsAgentBack["clubID"] = pClub->getClubID();
				getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
			}
			);
		}
		else {
			auto pClub = new Club();
			auto jsCreateOpts = jsReqContent["opts"];
			auto sCreateName = jsReqContent["name"].asString();
			pClub->init(this, generateClubID(), sCreateName, jsCreateOpts, 1000, 8);
			pClub->addMember(pPlayer->getUserUID(), eClubPrivilige_Creator);
			m_vClubs[pClub->getClubID()] = pClub;
			pPlayer->getBaseData()->onCreatedClub(pClub->getClubID());

			// save to db 
			Json::StyledWriter jsw;
			auto strOpts = jsw.write(jsCreateOpts);
			Json::Value jssql;
			char pBuffer[2048] = { 0 };
			sprintf_s(pBuffer, sizeof(pBuffer), "insert into clubs ( clubID,ownerUID,name,opts ) values ( %u,%u,'%s','%s');", pClub->getClubID(), pClub->getCreatorUID(), sCreateName.c_str(), strOpts.c_str());
			jssql["sql"] = pBuffer;
			getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Add, jssql);
			LOGFMTE("player uid = %u create club = %u ? ", nUID, pClub->getClubID());

			Json::Value jsAgentBack;
			jsAgentBack["ret"] = 0;
			jsAgentBack["clubID"] = pClub->getClubID();
			getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack);
		}
		return true;
	}
	return false;
}

bool ClubManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	if ( eAsync_HttpCmd_AddClubDiamond == nRequestType )
	{
		if ( jsReqContent["targetUID"].isNull() || jsReqContent["addCnt"].isNull() || jsReqContent["agentID"].isNull() )
		{
			jsResult["ret"] = 1;
			return true;
		}

		auto nClubID = jsReqContent["targetUID"].asUInt();
		auto iterClub = m_vClubs.find(nClubID);
		if (iterClub == m_vClubs.end())
		{
			jsResult["ret"] = 2;
			return true;
		}

		auto nAddCnt = jsReqContent["addCnt"].asInt();
		if (nAddCnt < 0 && abs(nAddCnt) > iterClub->second->getDiamond())
		{
			jsResult["ret"] = 3;
			return true;
		}

		jsResult["ret"] = 0;
		jsResult["addCnt"] = nAddCnt;
		iterClub->second->updateDiamond(nAddCnt);
		LOGFMTD( "agent id = %u add diamond = %d to clubid = %u", jsReqContent["agentID"].asUInt(),nAddCnt, nClubID );

		auto jsd = jsReqContent;
		CPlayer::saveDiamondRecorder(nClubID, eLogDiamond_ClubAgent, nAddCnt, iterClub->second->getDiamond(), jsd);
		return true;
	}

	if ( eAsync_HttpCmd_UpdateClubPointRestrict == nRequestType )  // check argument 
	{
		if (jsReqContent["clubID"].isNull() || jsReqContent["isEanble"].isNull() )
		{
			jsResult["ret"] = 1;
			return true;
		}

		auto nClubID = jsReqContent["clubID"].asUInt();
		auto iterClub = m_vClubs.find(nClubID);
		if (iterClub == m_vClubs.end())
		{
			jsResult["ret"] = 2;
			return true;
		}

		iterClub->second->setIsEnablePointRestrict( jsReqContent["isEanble"].asUInt() == 1 );
		jsResult = jsReqContent;
		jsResult["ret"] = 0;
		return true;
	}

	for (auto& ref : m_vClubs)
	{
		if ( ref.second->onAsyncRequest(nRequestType, jsReqContent, jsResult) )
		{
			return true;
		}
	}
	return false;
}

void ClubManager::onConnectedSvr( bool isReconnected )
{
	// read max clubID ;
	Json::Value jssql;
	jssql["sql"] = "select max(clubID) as 'maxClubID' from clubs ;";
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100,eAsync_DB_Select, jssql, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		uint8_t nRow = retContent["afctRow"].asUInt();
		Json::Value jsData = retContent["data"];
		if (nRow == 0)
		{
			LOGFMTD("do not read max serial number");
		}
		else
		{
			Json::Value jsRow = jsData[(uint32_t)0];
			m_nMaxClubID = jsRow["maxClubID"].asUInt();
#ifdef _DEBUG
			//m_nMaxClubID = 0;
#endif // _DEBUG
			LOGFMTD("read max serial number = %u", m_nMaxClubID);
		}
	});

	// read clubs row ;
	if ( m_vClubs.size() == 0 )
	{
		readClubs(0);
	}
}

void ClubManager::readClubs(uint32_t nAlreadyReadCnt)
{
	auto asyq = getSvrApp()->getAsynReqQueue();
	std::ostringstream ss;
	ss << "SELECT clubID,name,opts,state,cprState,diamond ,notice FROM clubs where isDelete = 0 limit 10 OFFSET " << nAlreadyReadCnt << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData ,bool isTimeOut) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTD("finish clubs ");
			finishReadClubs();
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];

			auto nClubID = jsRow["clubID"].asUInt();
			if ( m_vClubs.find(nClubID) != m_vClubs.end() )
			{
				LOGFMTE( "why have duplicate clubid = %u, stop read ",nClubID );
				finishReadClubs();
				return;
			}

			auto p = new Club();
			Json::Reader jsr;
			Json::Value jsOpts;
			jsr.parse(jsRow["opts"].asString(), jsOpts );
			p->init(this, jsRow["clubID"].asUInt(), jsRow["name"].asString(), jsOpts, 1000, 8,jsRow["state"].asUInt(), jsRow["cprState"].asUInt(), jsRow["diamond"].asUInt(),jsRow["notice"].asString());
			m_vClubs[p->getClubID()] = p;
		}

		auto nNewOffset = m_vClubs.size();
		if ( nNewOffset % 10 != 0 || nAft < 10 )  // finish read clubs ;
		{
			LOGFMTD("finish clubs 2 ");
			finishReadClubs();
			return;
		}

		// not finish , go on read 
		readClubs(nNewOffset);
	});
}

void ClubManager::finishReadClubs()
{
	for (auto& ref : m_vClubs)
	{
		ref.second->readClubDetail();
	}
}

void ClubManager::onTimeSave()
{
	for (auto& ref : m_vClubs)
	{
		ref.second->onTimeSave();
	}
}

bool ClubManager::canPlayerCreateClub(uint32_t nUID)
{
	return true;
}

uint32_t ClubManager::generateClubID()
{
	m_nMaxClubID = m_nMaxClubID + 1 + rand() % 15;
	return m_nMaxClubID;
}

void ClubManager::update(float fDeta)
{
	IGlobalModule::update(fDeta);
	for (auto & ref : m_vClubs)
	{
		ref.second->update(fDeta);
	}
}

bool ClubManager::isNameDuplicate(std::string& strName)
{
	for (auto& ref : m_vClubs)
	{
		if (ref.second->getName() == strName)
		{
			return true;
		}
	}
	return false;
}

uint32_t ClubManager::parePortTypte(uint32_t nRoomID)
{
	// begin(2) , portTypeCrypt (2),commonNum(2)
	int32_t nComNum = nRoomID % 100;
	int32_t portTypeCrypt = ((int32_t)(nRoomID / 100)) % 100;
	if (nComNum >= 50)
	{
		portTypeCrypt = portTypeCrypt + 100 - nComNum;
	}
	else
	{
		portTypeCrypt = portTypeCrypt + 100 + nComNum;
	}

	return (portTypeCrypt %= 100);
}

Club* ClubManager::getClub(uint32_t nClubID) {
	for (auto& ref : m_vClubs) {
		if (ref.first == nClubID) {
			return ref.second;
		}
	}
	return nullptr;
}