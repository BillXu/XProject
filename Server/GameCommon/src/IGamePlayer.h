#pragma once
#include "NativeTypes.h"
#include "json\json.h"
#include "stEnterRoomData.h"
#include "CommonDefine.h"
#include <memory>
#include "IGameRecorder.h"
class IPlayerRecorder;
class IGamePlayer
{
public:
	virtual ~IGamePlayer(){}
	virtual void init(stEnterRoomData* pEnterPlayer, uint16_t nIdx )
	{
		m_nSessionID = pEnterPlayer->nSessionID;
		m_nUserUID = pEnterPlayer->nUserUID;
		m_nIdx = nIdx;
		m_nWaiBaoOffset = 0;
		m_isOnline = true;
		m_nState = 0;
		m_nCurOffset = 0;
		m_nChips = pEnterPlayer->nChip;
		setState(eRoomPeer_WaitNextGame);
	}

	void setNewSessionID(uint32_t nNewSessionID)
	{
		m_nSessionID = nNewSessionID;
		m_isOnline = true;
	}

	uint32_t getUserUID()
	{
		return m_nUserUID;
	}

	uint32_t getSessionID()
	{
		return m_nSessionID;
	}

	int32_t getSingleOffset()
	{
		return m_nCurOffset;
	}

	int32_t getSingleWaiBaoOffset()
	{
		return m_nWaiBaoOffset;
	}

	int32_t addSingleOffset(int32_t nOffset)
	{
		m_nCurOffset += nOffset;
		m_nChips += nOffset;
		return m_nCurOffset;
	}
	int32_t getChips()
	{
		return m_nChips;
	}
	void setState(uint32_t eState) 
	{
		m_nState = eState;
	};

	bool haveState(uint32_t eState)
	{
		return (m_nState & eState) == eState;
	}

	void addState( uint32_t eState )
	{
		m_nState |= eState;
	}

	void clearState( uint32_t eState )
	{
		m_nState &= (~eState);
	}

	uint32_t getState()
	{
		return m_nState;
	}

	bool isTuoGuan()
	{
		return haveState(eRoomPeer_SysAutoAct);
	}

	void setTuoGuanFlag(uint8_t isTuoGuan)
	{
		if (isTuoGuan)
		{
			addState(eRoomPeer_SysAutoAct);
		}
		else
		{
			clearState(eRoomPeer_SysAutoAct);
		}
	}

	virtual void onGameWillStart(){ m_nCurOffset = 0; };
	virtual void onGameStart()
	{
		if ( haveState(eRoomPeer_Ready))
		{
			setState(eRoomPeer_CanAct);
		}
		
	};
	virtual void onGameEnd() {};
	virtual void onGameDidEnd()
	{ 
		m_nCurOffset = 0; 
		if ( false == haveState(eRoomPeer_Ready) )
		{ 
			setState(eRoomPeer_WaitNextGame);
		}
	 }

	uint16_t getIdx() { return m_nIdx; }
	void setIdx(uint16_t idx) { m_nIdx = idx; }
	bool isOnline() { return m_isOnline; }
	virtual void setIsOnline(bool isOnline) { m_isOnline = isOnline; }
	virtual bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)
	{
		ptrPlayerReocrder->setRecorder(getUserUID(), getSingleOffset());
		return true;
	}
private:
	bool m_isOnline = true;
	uint32_t m_nSessionID;
	uint32_t m_nUserUID;
	int32_t m_nCurOffset;
	int32_t m_nWaiBaoOffset;
	uint16_t m_nIdx;
	int32_t m_nChips;
	uint32_t m_nState;
};