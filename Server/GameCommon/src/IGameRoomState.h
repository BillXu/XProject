#pragma once
#include "json/json.h"
#include "NativeTypes.h"
#include "messageIdentifer.h"
class GameRoom;
class IGameRoomState
{
public:
	IGameRoomState() { m_fStateDuring = 999999; m_pRoom = nullptr; m_nNextStateID = -1; };
	IGameRoomState(uint32_t nNextState) { m_nNextStateID = nNextState; m_fStateDuring = 999999; m_pRoom = nullptr; }
	virtual ~IGameRoomState(){}
	virtual void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		m_pRoom = pmjRoom;
		setStateDuringTime(999999);
	}
	virtual uint32_t getStateID() = 0;
	virtual bool onMsg( Json::Value& jsmsg,uint16_t nMsgType,eMsgPort eSenderPort, uint32_t nSessionID ){ return false; }
	virtual void onStateTimeUp(){}
	virtual void leaveState(){}
	virtual uint8_t getCurIdx(){ return 0; };
	virtual void roomInfoVisitor(Json::Value& js) {}
	virtual void update(float fDeta)
	{
		if (m_fStateDuring >= 0.0f)
		{
			m_fStateDuring -= fDeta;
			if (m_fStateDuring <= 0.0f)
			{
				onStateTimeUp();
			}
		}
	}
	void setStateDuringTime(float fTime){ m_fStateDuring = fTime; }
	float getStateDuring(){ return m_fStateDuring; }
	GameRoom* getRoom(){ return m_pRoom; }
	uint32_t getNextStateID() { return m_nNextStateID; }
	virtual void checkTuoGuan(){}
private:
	float m_fStateDuring;
	GameRoom* m_pRoom;
	uint32_t m_nNextStateID;
};