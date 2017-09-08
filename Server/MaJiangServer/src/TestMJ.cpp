#include "TestMJ.h"
#include "TestMJPlayer.h"
bool TestMJ::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IMJRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);
}

IGamePlayer* TestMJ::createGamePlayer()
{
	return new TestMJPlayer();
}

uint8_t TestMJ::getRoomType()
{

}

IPoker* TestMJ::getPoker()
{

}