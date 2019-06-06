#include "TestMJ.h"
#include "TestMJPlayer.h"
#include "CommonDefine.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "MJRoomStateWaitPlayerAct.h"
#include "MJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "MJRoomStateDoPlayerAct.h"
#include "MJRoomStateAskForRobotGang.h"
#include "MJRoomStateAskForPengOrHu.h"
bool TestMJ::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,ptrGameOpts);
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new MJRoomStateWaitPlayerChu(),new MJRoomStateWaitPlayerAct(),new MJRoomStateStartGame(),new MJRoomStateGameEnd(),new MJRoomStateDoPlayerAct(),new MJRoomStateAskForRobotGang(),new MJRoomStateAskForPengOrHu()};
	for ( auto& pS : p )
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* TestMJ::createGamePlayer()
{
	return new TestMJPlayer();
}

uint8_t TestMJ::getRoomType()
{
	return eGame_TestMJ;
}

IPoker* TestMJ::getPoker()
{
	return &m_tPoker;
}