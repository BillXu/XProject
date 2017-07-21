#pragma once 
#include "IGameRoomState.h"
#include "IMJRoom.h"
class MJRoomStateStartGame
	:public IGameRoomState
{
public:
	uint32_t getStateID(){ return eRoomState_StartGame; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onWillStartGame();
		getRoom()->onStartGame();
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		Json::Value jsValue;
		jsValue["idx"] = ((IMJRoom*)getRoom())->getBankerIdx();
		getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
	}
	uint8_t getCurIdx()override{ return ((IMJRoom*)getRoom())->getBankerIdx(); }
};

