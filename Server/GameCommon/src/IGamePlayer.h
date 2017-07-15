#pragma once
#include "NativeTypes.h"
#include "json\json.h"
#include "stEnterRoomData.h"
class IGamePlayer
{
public:
	virtual ~IGamePlayer(){}
	virtual void init(stEnterRoomData* pEnterPlayer, uint16_t nIdx )
	{
		m_nSessionID = pEnterPlayer->nSessionID;
		m_nUserUID = pEnterPlayer->nUserUID;
		m_nIdx = nIdx;
		m_isOnline = true;
	}

	void setNewSessionID(uint32_t nNewSessionID)
	{
		m_nSessionID = nNewSessionID;
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

	int32_t addSingleOffset(int32_t nOffset)
	{
		m_nCurOffset += nOffset;
	}
	int32_t getChips()
	{
		return m_nChips;
	}

	virtual void onGameWillStart(){ m_nCurOffset = 0; };
	virtual void onGameStart() {};
	virtual void onGameEnd() {};
	virtual void onGameDidEnd() { m_nCurOffset = 0; };
	uint16_t getIdx() { return m_nIdx; }
	bool isOnline() { return m_isOnline; }
	void setIsOnline(bool isOnline) { m_isOnline = isOnline; }
private:
	bool m_isOnline = true;
	uint32_t m_nSessionID;
	uint32_t m_nUserUID;
	int32_t m_nCurOffset;
	uint16_t m_nIdx;
	int32_t m_nChips;
};