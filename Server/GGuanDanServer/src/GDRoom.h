#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "CardPoker.h"
#include "GuanDanDefine.h"
class GDRoom
	:public GameRoom
{
public:
	struct stLastGameInfo
	{
	public:
		void clear() {
			n1YouUID = 0;
			n2YouUID = 0;
			n3YouUID = 0;
			n4YouUID = 0;
			bSet = false;
		}

		uint32_t get1You() {
			return n1YouUID;
		}
		uint32_t get2You() {
			return n2YouUID;
		}
		uint32_t get3You() {
			return n3YouUID;
		}
		uint32_t get4You() {
			return n4YouUID;
		}
		void set1You(uint32_t n1You) {
			n1YouUID = n1You;
			bSet = true;
		}
		void set2You(uint32_t n2You) {
			n2YouUID = n2You;
			bSet = true;
		}
		void set3You(uint32_t n3You) {
			n3YouUID = n3You;
			bSet = true;
		}
		void set4You(uint32_t n4You) {
			n4YouUID = n4You;
			bSet = true;
		}
		bool isSet() {
			return bSet;
		}

	protected:
		uint32_t n1YouUID = 0;
		uint32_t n2YouUID = 0;
		uint32_t n3YouUID = 0;
		uint32_t n4YouUID = 0;
		bool bSet = false;
	};

public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	uint8_t getRoomType()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool canStartGame()override;
	IPoker* getPoker()override;
	bool isRoomFull()override { return false; }
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override { return 0; }
	uint8_t checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)override;

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool isCanDouble();
	uint8_t getBaseScore();
	bool isEnableRandomSeat();
	bool isEnableRandomDa();

	void doRandomChangeSeat();
	bool onWaitPayTribute(std::vector<uint8_t>& vWaitIdx);
	void doPayTribute(std::map<uint8_t, uint8_t>& mDoIdx, std::map<uint8_t, uint8_t>& mBackInfo);

	uint8_t getDaJi() { return m_nDaJi; }
	uint8_t getFirstChu() { return m_nFirstChu; }

protected:
	bool doChangeSeat(uint16_t nIdx, uint16_t nWithIdx);
	void confirmDaJi();

private:
	uint8_t m_nFirstChu;
	CGuanDanPoker m_tPoker;
	stLastGameInfo m_stLastGameInfo;
	uint8_t m_vDaJi[2];
	uint8_t m_nDaJi;
};