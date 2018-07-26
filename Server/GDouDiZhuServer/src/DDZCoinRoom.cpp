#include "DDZCoinRoom.h"
#include "DDZRoom.h"
#include "IGamePlayer.h"
bool DDZCoinRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	ICoinRoom::onPlayerEnter(pEnterRoomPlayer);
	for (auto nidx = 0; nidx < getSeatCnt(); ++nidx)
	{
		if (getPlayerByIdx(nidx) == nullptr)
		{
			getCoreRoom()->doPlayerSitDown(pEnterRoomPlayer,nidx);
			return true;
		}
	}
	return false;
}

void DDZCoinRoom::onGameDidEnd(IGameRoom* pRoom)
{
	ICoinRoom::onGameDidEnd(pRoom);
	for (auto nidx = 0; nidx < getSeatCnt(); ++nidx)
	{
		auto p = getPlayerByIdx(nidx);
		if ( p )
		{
			getCoreRoom()->doPlayerLeaveRoom( p->getUserUID() );
		}
	}
}

GameRoom* DDZCoinRoom::doCreatRealRoom()
{
	return new DDZRoom();
}