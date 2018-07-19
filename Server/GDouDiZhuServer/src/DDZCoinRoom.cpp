#include "DDZCoinRoom.h"
#include "DDZRoom.h"
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

GameRoom* DDZCoinRoom::doCreatRealRoom()
{
	return new DDZRoom();
}