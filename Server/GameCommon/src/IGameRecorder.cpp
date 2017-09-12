#include "IGameRecorder.h"
#include "log4z.h"
#include "AsyncRequestQuene.h"
void ISingleRoundRecorder::init(uint16_t nRoundIdx, uint32_t nFinish, uint32_t nReplayID)
{
	m_vPlayerRecorderInfo.clear();
	m_nRoundIdx = nRoundIdx;
	m_nFinishTime = nFinish;
	m_nReplayID = nReplayID;
}

uint16_t ISingleRoundRecorder::getRoundIdx()
{
	return m_nRoundIdx;
}

uint32_t ISingleRoundRecorder::getFinishTime()
{
	return m_nFinishTime;
}

uint32_t ISingleRoundRecorder::getReplayID()
{
	return m_nReplayID;
}

bool ISingleRoundRecorder::addPlayerRecorderInfo( std::shared_ptr<IPlayerRecorder> ptrPlayerRecorderInfo )
{
	auto iter = m_vPlayerRecorderInfo.find(ptrPlayerRecorderInfo->getUserUID());
	if (iter != m_vPlayerRecorderInfo.end())
	{
		LOGFMTE("already have this player recorder uid = %u , offset = %d ", ptrPlayerRecorderInfo->getUserUID(), ptrPlayerRecorderInfo->getOffset() );
		return false;
	}
	m_vPlayerRecorderInfo[ptrPlayerRecorderInfo->getUserUID()] = ptrPlayerRecorderInfo;
	return true;
}

void ISingleRoundRecorder::doSaveRoomRecorder( IGameRoomRecorder* pOwnRoomRecorder, CAsyncRequestQuene* pSyncQuene )
{
	if (m_vPlayerRecorderInfo.empty())
	{
		LOGFMTE( "room sieral = %u , round = %u do not have player , so do not save .",pOwnRoomRecorder->getSieralNum(),m_nRoundIdx );
		return;
	}

	Json::Value jsPlayers;
	for (auto& ref : m_vPlayerRecorderInfo)
	{
		Json::Value jsPlayer;
		ref.second->toJson(jsPlayer);
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}

	// to string 
	std::string strPlayers;
	if (jsPlayers.isNull() == false)
	{
		Json::StyledWriter jswrite;
		strPlayers = jswrite.write(jsPlayers);
	}


	// do save sql  room round recorder 
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "insert into room_record_per_round ( sieralNum,roundIdx,replayID,time,resultDetail ) values (%u,%u,%u,from_unixtime( %u ),",pOwnRoomRecorder->getSieralNum(),m_nRoundIdx,m_nReplayID,m_nFinishTime );
	std::ostringstream ss;
	ss << pBuffer << " '" << jsPlayers << "' ) ;";
	jssql["sql"] = ss.str();
	pSyncQuene->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, pOwnRoomRecorder->getSieralNum(), eAsync_DB_Add, jssql);
}

bool ISingleRoundRecorder::calculatePlayerTotalOffset(std::map<uint32_t, int32_t>& vPlayerOffset)
{
	for (auto& ref : m_vPlayerRecorderInfo)
	{
		auto iter = vPlayerOffset.find(ref.first);
		if (iter == vPlayerOffset.end())
		{
			vPlayerOffset[ref.first] = ref.second->getOffset();
		}
		else
		{
			iter->second += ref.second->getOffset();
		}
	}

	return true;
}
// room recorder 
void IGameRoomRecorder::init( uint32_t nSieralNum, uint32_t nRoomID, uint32_t nRoomType, uint32_t nCreaterUID, Json::Value& jsOpts )
{
	m_nRoomID = nRoomID;
	m_nSieralNum = nSieralNum;
	m_nRoomType = nRoomType;
	m_nCreaterUID = nCreaterUID;
	m_jsOpts = jsOpts;
	m_vAllRoundRecorders.clear();
}

void IGameRoomRecorder::addSingleRoundRecorder(std::shared_ptr<ISingleRoundRecorder>& ptrSingleRecorder)
{
	if (ptrSingleRecorder == nullptr)
	{
		LOGFMTE("add single round offset is nullptr");
		return;
	}

	auto iter = m_vAllRoundRecorders.find(ptrSingleRecorder->getRoundIdx());
	if (iter != m_vAllRoundRecorders.end())
	{
		LOGFMTE("duplicate round recorder for idx = %u", ptrSingleRecorder->getRoundIdx());
		m_vAllRoundRecorders.erase(iter);
	}
	m_vAllRoundRecorders[ptrSingleRecorder->getRoundIdx()] = ptrSingleRecorder;
}

std::shared_ptr<ISingleRoundRecorder> IGameRoomRecorder::getSingleRoundRecorder(uint16_t nRoundUIdx)
{
	auto iter = m_vAllRoundRecorders.find(nRoundUIdx);
	if (iter == m_vAllRoundRecorders.end())
	{
		return nullptr;
	}
	return iter->second;
}

uint32_t IGameRoomRecorder::getSieralNum()
{
	return m_nSieralNum;
}

uint16_t IGameRoomRecorder::getRoundRecorderCnt()
{
	return m_vAllRoundRecorders.size();
}

void IGameRoomRecorder::doSaveRoomRecorder( CAsyncRequestQuene* pSyncQuene )
{
	if ( m_vAllRoundRecorders.empty())
	{
		LOGFMTD( "room id = %u do not have recorder creator id = %u", m_nRoomID, m_nCreaterUID );
		return;
	}
	// get opts str 
	std::string strOpts = "";
	if ( m_jsOpts.isNull() == false )
	{
		Json::StyledWriter jswrite;
		strOpts = jswrite.write(m_jsOpts);
	}

	// do save sql  room recorder 
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer,sizeof(pBuffer) ,"insert into roominfo ( sieralNum,roomID,createUID,time,roomType,opts ) values (%u,%u,%u,now(),'%u',", m_nSieralNum, m_nRoomID,m_nCreaterUID, m_nRoomType );
	std::ostringstream ss;
	ss << pBuffer << "'" << strOpts << "' ) ;";
	jssql["sql"] = ss.str();
	pSyncQuene->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB,getSieralNum(), eAsync_DB_Add, jssql );

	// do save player recorder ;
	std::map<uint32_t, int32_t> vPlayerOffset;
	for (auto& ref : m_vAllRoundRecorders)
	{
		ref.second->calculatePlayerTotalOffset(vPlayerOffset);
	}

	// do save 
	for ( auto& ref : vPlayerOffset)
	{
		auto nUserUID = ref.first;
		auto nOffset = ref.second;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "insert into playerrecorder ( userUID,sieralNum,roomID,offset,time,roomType ) values (%u,%u,%u,%d,now(),'%u');", nUserUID,m_nSieralNum, m_nRoomID, nOffset, m_nRoomType);
		jssql["sql"] = pBuffer;
		pSyncQuene->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, getSieralNum(), eAsync_DB_Add, jssql);
	}

	// do save round info ;
	Json::Value jsRounds;
	for (auto& ref : m_vAllRoundRecorders)
	{
		ref.second->doSaveRoomRecorder(this, pSyncQuene);
	}
}

