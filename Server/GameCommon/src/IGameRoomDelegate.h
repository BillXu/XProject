#pragma once
#include "NativeTypes.h"
class IGameRoom;
class IGamePlayer;
class IGameRoomDelegate
{
public:
	virtual ~IGameRoomDelegate(){ }
	virtual void onWillStartGame(IGameRoom* pRoom) {};
	virtual void onStartGame(IGameRoom* pRoom) {};
	virtual bool canStartGame(IGameRoom* pRoom) { return true; }
	virtual void onGameDidEnd(IGameRoom* pRoom){ }
	virtual void onGameEnd( IGameRoom* pRoom ){}
	virtual void doPlayerEnter(IGameRoom* pRoom, uint32_t nUserUID) {}
	virtual void onPlayerWillSitDown(IGameRoom* pRoom, uint32_t nUserUID) {}
	virtual bool canPlayerSitDown(uint32_t nUserUID) { return true; }
	virtual void onPlayerSitDown(IGameRoom* pRoom,IGamePlayer* pPlayer ){}
	virtual void onPlayerWillStandUp(IGameRoom* pRoom,IGamePlayer* pPlayer) {}
	virtual void onPlayerStandedUp( IGameRoom* pRoom,uint32_t nUserUID) {}
	virtual void onPlayerDoLeaved( IGameRoom* pRoom, uint32_t nUserUID ) {}
	virtual void onPlayerTOut(uint32_t nUserUID) {}
	virtual bool isEnableRecorder() { return false; }
	virtual bool isEnableReplay() { return false; }
	virtual void setCurrentPointer(IGameRoom* pRoom) {}
	virtual void onPlayerWaitDragIn(uint32_t nUserUID) {}
	virtual void onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID) {}
	virtual bool isRoomGameOver() { return false; }
	virtual std::shared_ptr<IGameRoomRecorder> getRoomRecorder() { return nullptr; }
	virtual uint32_t getRoomPlayerCnt() { return 0; }
	virtual uint32_t getDragInClubID(uint32_t nUserID) { return 0; }
	virtual uint32_t getClubID() { return 0; }
	virtual uint32_t getLeagueID() { return 0; }
	virtual void onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin) {}
	virtual void onPlayerAddRBPool(IGamePlayer* pPlayer, int32_t nCoin) {}
	virtual void onPlayerAutoStandUp(uint32_t nUserUID, bool bSwitch = true) {}
	virtual void onPlayerAutoLeave(uint32_t nUserUID, bool bSwitch = true) {}
	virtual uint32_t getEnterClubID(uint32_t nUserUID) { return 0; }
};