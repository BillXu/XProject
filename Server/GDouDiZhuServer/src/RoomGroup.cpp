#include "RoomGroup.h"
#include "IGameRoom.h"
#include "IGameRoomManager.h"
#include "ICoinRoom.h"
#include "stEnterRoomData.h"
#include "Timer.h"
#include <algorithm>
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
uint32_t RoomGroup::s_MaxGroupRoomID = 1000000;
RoomGroup::RoomGroup()
{
	 
}

RoomGroup::~RoomGroup()
{
	m_tMaxDelayMatchPlayer.canncel();
	for (auto& ref : m_vAllRooms)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vAllRooms.clear();
}

bool RoomGroup::init(IGameRoomManager* pmgr, Json::Value& jsCreateRoomOpts)
{
	m_jsRoomOpts = jsCreateRoomOpts;
	m_pRoomMgr = pmgr;
	// default create 10 room ;
	uint16_t nCnt = 10;
	while ( nCnt-- )
	{
		auto pRoom = createRoom();
		m_vAllRooms[pRoom->getRoomID()] = pRoom;
	}

	// set up check delay match player 
	m_tMaxDelayMatchPlayer.setInterval(2);
	m_tMaxDelayMatchPlayer.setIsAutoRepeat(true);
	m_tMaxDelayMatchPlayer.setCallBack([this](CTimer* p, float fDelta)
	{
		if ( doMatchPlayerEnterRoom() ) // if all players matched , then canncel timer ; 
		{
			m_tMaxDelayMatchPlayer.canncel();
		}
	});
	return true;
}

void RoomGroup::update(float fdeta)
{
	for (auto& ref : m_vAllRooms)
	{
		ref.second->update(fdeta);
	}
}

bool RoomGroup::pushPlayerToEnterRoomQueue(stEnterRoomData& pPlayer)
{
	if ( pPlayer.isRobot )
	{
		m_vEnterRoomRobotQuene.push_back(pPlayer);
		return true;
	}

	m_vEnterRoomQuene.push_back(pPlayer);
	if ( m_vEnterRoomQuene.size() >= 12 )
	{
		if ( doMatchPlayerEnterRoom() )  // if all players matched , then reset timer ;
		{
			m_tMaxDelayMatchPlayer.reset();
		}
	}
	else if ( 1 == m_vEnterRoomQuene.size() )  // start wait enter room 
	{
		m_tMaxDelayMatchPlayer.reset();
		m_tMaxDelayMatchPlayer.start();
	}
	return true;
}

uint8_t RoomGroup::checkEnterThisLevel(stEnterRoomData& pPlayer)
{
	auto nEnterLimitLow = m_jsRoomOpts["enterLimitLow"].asInt();
	auto nEnterLimitTop = m_jsRoomOpts["enterLimitTop"].asInt();
	if ( nEnterLimitLow && pPlayer.nChip < nEnterLimitLow )
	{
		return 1;
	}

	if ( nEnterLimitTop && pPlayer.nChip > nEnterLimitTop )
	{
		return 2;
	}

	return 0;
}

uint32_t RoomGroup::getLevel()
{
	return m_jsRoomOpts["level"].asUInt();
}

IGameRoom* RoomGroup::getRoomByID(uint32_t nRoomID)
{
	auto iter = m_vAllRooms.find(nRoomID);
	if (iter != m_vAllRooms.end())
	{
		return iter->second;
	}
	return nullptr;
}

bool RoomGroup::removePlayerFromQueue(uint32_t nUID)
{
	auto iter = m_vEnterRoomQuene.begin();
	for (; iter != m_vEnterRoomQuene.end(); ++iter)
	{
		if ((*iter).nUserUID == nUID)
		{
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			// clear queueing state 
			Json::Value jsReqLeave;
			jsReqLeave["targetUID"] = (*iter).nUserUID;
			jsReqLeave["level"] = -1;
			jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, (*iter).nUserUID, eAsync_Set_Queuing_CoinGameLevel, jsReqLeave);

			m_vEnterRoomQuene.erase(iter);
			return true;
		}
	}

	iter = m_vEnterRoomRobotQuene.begin();
	for (; iter != m_vEnterRoomRobotQuene.end(); ++iter)
	{
		if ((*iter).nUserUID == nUID)
		{
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			// clear queueing state 
			Json::Value jsReqLeave;
			jsReqLeave["targetUID"] = (*iter).nUserUID;
			jsReqLeave["level"] = -1;
			jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, (*iter).nUserUID, eAsync_Set_Queuing_CoinGameLevel, jsReqLeave);

			m_vEnterRoomRobotQuene.erase(iter);
			return true;
		}
	}

	return false;
}

bool RoomGroup::updatePlayerSessionID(uint32_t nUID, uint32_t nNewSessionID)
{
	auto iter = m_vEnterRoomQuene.begin();
	for (; iter != m_vEnterRoomQuene.end(); ++iter)
	{
		if ((*iter).nUserUID == nUID)
		{
			(*iter).nSessionID = nNewSessionID;
			return true;
		}
	}

	iter = m_vEnterRoomRobotQuene.begin();
	for (; iter != m_vEnterRoomRobotQuene.end(); ++iter)
	{
		if ((*iter).nUserUID == nUID)
		{
			(*iter).nSessionID = nNewSessionID;
			return true;
		}
	}
	return false;
}

IGameRoom* RoomGroup::createRoom()
{
	auto pRoom = new ICoinRoom();
	pRoom->init(m_pRoomMgr, m_pRoomMgr->generateSieralID(), ++s_MaxGroupRoomID, m_jsRoomOpts["seatCnt"].asUInt(), m_jsRoomOpts);
	return pRoom;
}

bool RoomGroup::doMatchPlayerEnterRoom()
{
	const uint16_t nSeatCnt = 3;
	auto isEnough = m_vEnterRoomQuene.size() >= 9 && ( m_vEnterRoomQuene.size() % nSeatCnt == 0 );
	if ( isEnough == false ) // add robot
	{
		auto nNeedRobotCnt = nSeatCnt - m_vEnterRoomQuene.size() % nSeatCnt;
		while ( nNeedRobotCnt-- && ( m_vEnterRoomRobotQuene.empty() == false ) )
		{
			m_vEnterRoomQuene.push_back( m_vEnterRoomRobotQuene.front() );
			m_vEnterRoomRobotQuene.pop_front();
		}
	}

	std::random_shuffle(m_vEnterRoomQuene.begin(), m_vEnterRoomQuene.end());
	// inform players enter room ;
	uint32_t nNeedRoomCnt = m_vEnterRoomQuene.size() / nSeatCnt;
	std::vector<IGameRoom*> vRooms;
	for ( auto iter = m_vAllRooms.begin(); iter != m_vAllRooms.end() && vRooms.size() < nNeedRoomCnt; ++iter )
	{
		auto pRoom = iter->second;
		if ( pRoom && pRoom->getPlayerCnt() == 0 )
		{
			vRooms.push_back(pRoom);
		}
	}

	while ( vRooms.size() < nNeedRoomCnt )
	{
		vRooms.push_back( createRoom() );
	}

	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	// put players into room and send room info ;
	for (auto& ref : vRooms)
	{
		auto nCnt = nSeatCnt;
		while ( nCnt-- )
		{
			auto p = m_vEnterRoomQuene.front();
			if (ref->onPlayerEnter(&p))
			{
				ref->sendRoomInfo(p.nSessionID);
			}
			m_vEnterRoomQuene.pop_front();

			// clear queueing state 
			Json::Value jsReqLeave;
			jsReqLeave["targetUID"] = p.nUserUID;
			jsReqLeave["level"] = -1;
			jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, p.nUserUID, eAsync_Set_Queuing_CoinGameLevel, jsReqLeave);

			// update stayin roomID
			jsReqLeave["roomID"] = ref->getRoomID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, p.nUserUID, eAsync_Inform_EnterRoom, jsReqLeave);
		}
	}
}