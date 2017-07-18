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
	virtual void onPlayerSitDown(IGameRoom* pRoom,IGamePlayer* pPlayer ){ }
	virtual void onPlayerWillStandUp(IGameRoom* pRoom,IGamePlayer* pPlayer) {};
	virtual void onPlayerStandedUp( IGameRoom* pRoom,uint32_t nUserUID) {} ;
	virtual void onPlayerDoLeaved( IGameRoom* pRoom, uint32_t nUserUID ) {};
	virtual bool isEnableRecorder() { return false; }
	virtual bool isEnableReplay() { return false; }
};