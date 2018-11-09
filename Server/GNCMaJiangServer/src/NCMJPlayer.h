#pragma once
#include "IMJPlayer.h"
#include "NCMJPlayerCard.h"
class NCMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
	void onGameWillStart()override;
	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
	void init(stEnterRoomData* pData, uint16_t nIdx)override;
	void clearGangFlag() { clearFlag(eMJActFlag_Gang); }
	bool haveGangFlag() { return haveFlag(eMJActFlag_Gang); }

	bool isGangHouPao() { return m_bGangHouPao; }
	void signGangHouPao() { m_bGangHouPao = true; }
	void clearGangHouPao() { m_bGangHouPao = false; m_nLastGangType = eMJAct_None; }
	eMJActType getLaseGangType() { return m_nLastGangType; }
	void setLastGangType(eMJActType gangType) { m_nLastGangType = gangType; }
protected:
	NCMJPlayerCard m_tPlayerCard;
	float m_nExtraTime;

	bool m_bGangHouPao;
	eMJActType m_nLastGangType;
};