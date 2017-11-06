#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "DDZRoomStateWaitReady.h"
#include "DDZRoomStatePlayerChu.h"
#include "DDZRoomStateStartGame.h"
#include "DDZRoomStateRobotBanker.h"
#include "DDZRoomStateGameEnd.h"
bool DDZRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_nFirstRobotBankerIdx = rand() % getSeatCnt();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;

	IGameRoomState* pState = new DDZRoomStateWaitReady();
	addRoomState(pState);
	setInitState(pState);
	pState = new DDZRoomStatePlayerChu();
	addRoomState(pState);
	pState = new DDZRoomStateStartGame();
	addRoomState(pState);
	pState = new DDZRoomStateRobotBanker();
	addRoomState(pState);
	pState = new DDZRoomStateGameEnd();
	addRoomState(pState);
	return true;
}

IGamePlayer* DDZRoom::createGamePlayer()
{
	return new DDZPlayer();
}

void DDZRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
}

void DDZRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	GameRoom::visitPlayerInfo(pPlayer,jsPlayerInfo,nVisitorSessionID);
}

uint8_t DDZRoom::getRoomType()
{
	return eGame_CYDouDiZhu;
}

void DDZRoom::onStartGame()
{
	GameRoom::onStartGame();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;
	m_vDiPai.clear();
}

void DDZRoom::onGameEnd()
{
	GameRoom::onGameEnd();
}

bool DDZRoom::canStartGame()
{
	if (GameRoom::canStartGame() == false)
	{
		return false;
	}

	for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (nullptr == p)
		{
			continue;
		}

		if (p->haveState(eRoomPeer_Ready) == false)
		{
			return false;
		}
	}
	return true;
}

IPoker* DDZRoom::getPoker()
{
	return &m_tPoker;
}

bool DDZRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if (GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID))
	{
		return true;
	}
	return false;
}

uint8_t DDZRoom::getFirstRobotBankerIdx()
{
	return m_nFirstRobotBankerIdx = (++m_nFirstRobotBankerIdx) % getSeatCnt();
}

void DDZRoom::setNewBankerInfo(uint8_t nBankerIdx, uint8_t nBankerTimes, std::vector<uint8_t>& vDiPai)
{
	m_nBankerIdx = nBankerIdx;
	m_nBankerTimes = nBankerTimes;
	m_vDiPai = vDiPai;
}

uint8_t DDZRoom::getBankTimes()
{
	return m_nBankerTimes;
}

uint8_t DDZRoom::getBombCount()
{
	return m_nBombCnt;
}

void DDZRoom::increaseBombCount()
{
	++m_nBombCnt;
}