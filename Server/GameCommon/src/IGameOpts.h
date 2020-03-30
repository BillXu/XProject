#pragma once
#include "NativeTypes.h"
#include "json/json.h"
#include <memory>
class IGameOpts
{
public:
	uint32_t getRoomID() { return m_nRoomID; };
	void setRoomID(uint32_t nRoomID) { m_nRoomID = nRoomID; }
	Json::Value getJSOpts() { return m_jsOpts; }

	virtual void initRoomOpts(Json::Value& jsOpts);
	uint32_t getGameType() { return m_nGameType; }
	uint8_t getSeatCnt() { return m_nSeatCnt; }
	void setSeatCnt(uint8_t nSeatCnt) { m_nSeatCnt = nSeatCnt; }
	uint8_t getPayType() { return m_nPayType; }
	bool isDK() { return m_bDaiKai; }
	uint32_t getVipLevel() { return m_nVipLevel; }
	uint32_t getAutoStartCnt() { return m_nAutoStartCnt; }
	uint32_t getOwnerUID() { return m_nOwnerUID; }
	uint32_t getClubID() { return m_nClubID; }
	uint8_t getRoundLevel() { return m_nRoundLevel; }
	bool isEnableWhiteList() { return m_bEnableWhiteList; }
	bool isEnablePointRestrict() { return m_bEnablePointRestrict; }
	bool isAA() { return m_bAA; }

	uint16_t getDiamondNeed(); //vipLevel, initRound, payType
	uint8_t getInitRound(); //level
	virtual uint8_t calculateInitRound() = 0;
	virtual uint16_t calculateDiamondNeed() = 0;

	static std::shared_ptr<IGameOpts> parseOpts(Json::Value& jsOpts);
private:
	uint32_t m_nRoomID;
	uint32_t m_nClubID;
	uint32_t m_nOwnerUID;
	Json::Value m_jsOpts;

	uint32_t m_nGameType;
	uint8_t m_nSeatCnt;
	uint8_t m_nPayType;
	bool m_bDaiKai;
	uint32_t m_nVipLevel;
	uint32_t m_nAutoStartCnt;
	uint32_t m_nRoundLevel;
	bool m_bEnableWhiteList;
	bool m_bEnablePointRestrict;
	bool m_bAA;

	uint8_t m_nInitRound;
	bool m_bCalculateInitRound = false;
	uint16_t m_nDiamondNeed;
	bool m_bCalculateDiamondNeed = false;
};