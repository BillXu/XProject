#include "NJMJ.h"
#include "NJPlayer.h"
#include "CommonDefine.h"
#include "MJRoomStateWaitReady.h"
#include "MJRoomStateWaitPlayerChu.h"
#include "MJRoomStateWaitPlayerAct.h"
#include "MJRoomStateStartGame.h"
#include "MJRoomStateGameEnd.h"
#include "MJRoomStateDoPlayerAct.h"
#include "MJRoomStateAskForRobotGang.h"
#include "MJRoomStateAskForPengOrHu.h"
#define PUNISH_COIN_BASE 5 
#define AN_GANG_COIN_BASE 5 
#define MING_GANG_COIN_BASE 10
#define HU_GANG_COIN_BASE 10
bool NJMJ::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IMJRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	mOpts.init(vJsOpts);
	// add room state ;
	IGameRoomState* p[] = { new CMJRoomStateWaitReady(), new MJRoomStateWaitPlayerChu(),new MJRoomStateWaitPlayerAct(),new MJRoomStateStartGame(),new MJRoomStateGameEnd(),new MJRoomStateDoPlayerAct(),new MJRoomStateAskForRobotGang(),new MJRoomStateAskForPengOrHu() };
	for (auto& pS : p)
	{
		addRoomState(pS);
	}
	setInitState(p[0]);
	return true;
}

IGamePlayer* NJMJ::createGamePlayer()
{
	return new NJPlayer();
}

uint8_t NJMJ::getRoomType()
{
	return eGame_NJMJ;
}

IPoker* NJMJ::getPoker()
{
	return &m_tPoker;
}

bool NJMJ::isGameOver()
{
	if ( IMJRoom::isGameOver() )
	{
		return true;
	}

	if ( mOpts.optsJinYuanZi() )
	{
		uint8_t cnt = 0;
		for each ( auto var in m_vPlayers )
		{
			if (nullptr == var)
			{
				continue;
			}

			if ( var->getChips() <= 0 )
			{
				++cnt;

				if ( cnt >= 2 )
				{
					return true;
				}
			}

		}
	}
	return false;
}

void NJMJ::onPlayerChu(uint8_t nIdx, uint8_t nCard)
{
	IMJRoom::onPlayerChu( nIdx,nCard );
	m_tChuedCards.addChuedCard(nCard, nIdx);
	doProcessChuPaiFanQian(nIdx, nCard);
}

void NJMJ::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)
{
	IMJRoom::onPlayerMingGang(nIdx, nCard, nInvokeIdx);
	auto pActPlayer = (NJPlayer*)getPlayerByIdx(nIdx);
	auto pActCard = (NJPlayerCard*)pActPlayer->getPlayerCard();

	pActCard->setSongGangIdx(nInvokeIdx);

	//pActCard->addActSign(nCard, nInvokeIdx, eMJAct_MingGang);

	auto pInvokerPlayer = getPlayerByIdx(nInvokeIdx);

	// do cacualte ;
	stSettle st;
	st.eSettleReason = eMJAct_MingGang;
	uint16_t nLose = MING_GANG_COIN_BASE * (isBiXiaHu() ? 2 : 1);
	if (nLose > pInvokerPlayer->getChips())
	{
		nLose = pInvokerPlayer->getChips();
	}

	pInvokerPlayer->addSingleOffset(-1 * (int32_t)nLose);
	st.addLose(pInvokerPlayer->getIdx(), nLose);

	pActPlayer->addSingleOffset(nLose);
	st.addWin(nIdx, nLose);
	addSettle(st);
}

void NJMJ::onPlayerAnGang(uint8_t nIdx, uint8_t nCard)
{
	IMJRoom::onPlayerAnGang(nIdx, nCard);
	auto pPlayerWin = (NJPlayer*)getPlayerByIdx(nIdx);
	auto pActCard = (NJPlayerCard*)pPlayerWin->getPlayerCard();
	// do caculate ;
	stSettle st;
	st.eSettleReason = eMJAct_AnGang;
	uint16_t nWin = 0;
	uint16_t nLosePerPlayer = AN_GANG_COIN_BASE * (isBiXiaHu() ? 2 : 1);
	for (uint8_t nCheckIdx = 0; nCheckIdx < getSeatCnt(); ++nCheckIdx)
	{
		if (nIdx == nCheckIdx)
		{
			continue;
		}

		auto pPlayer = getPlayerByIdx(nCheckIdx);
		if ((uint8_t)-1 != pActCard->getSongGangIdx())
		{
			pPlayer = getPlayerByIdx(pActCard->getSongGangIdx());
		}

		uint16_t nLose = nLosePerPlayer;

		if (pPlayer->getChips() < nLose)
		{
			nLose = pPlayer->getChips();
		}
		pPlayer->addSingleOffset(-1 * (int32_t)nLose);
		st.addLose(pPlayer->getIdx(), nLose);
		nWin += nLose;
	}

	pPlayerWin->addSingleOffset(nWin);
	st.addWin(nIdx, nWin);
	addSettle(st);
	//pActCard->addActSign(nCard, nIdx, eMJAct_AnGang);
}

void NJMJ::onPlayerBuGang(uint8_t nIdx, uint8_t nCard)
{
	IMJRoom::onPlayerBuGang(nIdx, nCard);
	auto pActPlayer = (NJPlayer*)getPlayerByIdx(nIdx);
	auto pActCard = (NJPlayerCard*)pActPlayer->getPlayerCard();

	auto nInvokeIdx = pActCard->getSongGangIdx();
	if ((uint8_t)-1 == nInvokeIdx)
	{
		nInvokeIdx = pActCard->getInvokerPengIdx(nCard);
	}

	if ((uint8_t)-1 == nInvokeIdx)
	{
		LOGFMTE("room id = %u, bu gang do not have idx = %u peng = %u", getRoomID(), nIdx, nCard);
		return;
	}

	auto pInvokerPlayer = getPlayerByIdx(nInvokeIdx);

	// do cacualte ;
	stSettle st;
	st.eSettleReason = eMJAct_BuGang;
	uint16_t nLose = MING_GANG_COIN_BASE * (isBiXiaHu() ? 2 : 1);
	if (nLose > pInvokerPlayer->getChips())
	{
		nLose = pInvokerPlayer->getChips();
	}

	pInvokerPlayer->addSingleOffset(-1 * (int32_t)nLose);
	st.addLose(pInvokerPlayer->getIdx(), nLose);


	pActPlayer->addSingleOffset(nLose);
	st.addWin(nIdx, nLose);
	addSettle(st);
}

void NJMJ::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)
{

}

bool NJMJ::canPlayerHuWithCard(IMJPlayer* p, uint8_t nCard, uint8_t nInvokerIdx)
{

}

void NJMJ::addSettle(stSettle& tSettle)
{
	m_vSettle.push_back(tSettle);
	// do send message ;
	Json::Value jsMsg;
	jsMsg["actType"] = tSettle.eSettleReason;

	Json::Value jsWin;
	for (auto& ref : tSettle.vWinIdxs)
	{
		Json::Value js;
		js["idx"] = ref.first;
		js["offset"] = ref.second;
		js["isWin"] = 1;
		jsWin[jsWin.size()] = js;
	}
	jsMsg["winers"] = jsWin;

	Json::Value jsLose;
	for (auto& ref : tSettle.vLoseIdx)
	{
		Json::Value js;
		js["idx"] = ref.first;
		js["offset"] = ref.second;
		js["isWin"] = 0;
		jsLose[jsLose.size()] = js;
	}
	jsMsg["loserIdxs"] = jsLose;

	sendRoomMsg(jsMsg, MSG_ROOM_NJ_REAL_TIME_SETTLE);
}

void NJMJ::doProcessChuPaiFanQian( uint8_t chuIdx , uint8_t chuCard )
{
	auto nIdx = chuIdx;
	auto nCard = chuCard;
	// check chu 4 feng 
	{
		auto pChuPaiPlayer = (NJPlayer*)getPlayerByIdx(nIdx);
		auto pcard = (NJPlayerCard*)pChuPaiPlayer->getPlayerCard();
		if ( mOpts.optsSiLianFeng() && pcard->isChued4Feng())
		{
			// do caculate ;
			stSettle st;
			st.eSettleReason = eMJAct_4Feng;
			uint16_t nWin = 0;
			uint16_t nLosePerPlayer = AN_GANG_COIN_BASE * ( isBiXiaHu() ? 2 : 1);
			for (uint8_t nCheckIdx = 0; nCheckIdx < getSeatCnt(); ++nCheckIdx)
			{
				if (nIdx == nCheckIdx)
				{
					continue;
				}

				auto pPlayer = getPlayerByIdx(nCheckIdx);
				if (nullptr == pPlayer)
				{
					continue;
				}

				uint16_t nLose = nLosePerPlayer;

				if (pPlayer->getChips() < nLose)
				{
					nLose = pPlayer->getChips();
				}
				pPlayer->addSingleOffset(-1 * (int32_t)nLose);
				st.addLose(nCheckIdx, nLose);
				nWin += nLose;
			}

			auto pPlayerWin = getPlayerByIdx(nIdx);
			pPlayerWin->addSingleOffset(nWin);
			st.addWin(nIdx, nWin);
			addSettle(st);
			//m_isSiLianFengFaQian = true;
			m_isWillBiXiaHu = true;
			//return;
		}

	}

	uint8_t nFanQianTarget = -1;
	uint8_t nSettleType = 0;
	// reset songGang idx 
	auto pActPlayer = (NJPlayer*)getPlayerByIdx(nIdx);
	auto pcard = (NJPlayerCard*)pActPlayer->getPlayerCard();

	if ( getSeatCnt() == MAX_SEAT_CNT && m_tChuedCards.isInvokerFanQian(nFanQianTarget) )
	{
		nSettleType = eMJAct_Followed;
		LOGFMTD("room id = %u gen feng fa qian , card = %u , idx = %u", getRoomID(), nCard, nIdx);
	}
	else
	{
		if (!pcard->isChued4Card(nCard))
		{
			return;
		}

		nFanQianTarget = nIdx;
		nSettleType = eMJAct_Chu;

		LOGFMTD("room id = %u chu 4 ge pai , card = %u , idx = %u", getRoomID(), nCard, nIdx);
	}

	if ((uint8_t)-1 == nFanQianTarget)
	{
		return;
	}

	m_isWillBiXiaHu = true;
	// do fanqian logic
	auto pLosePlayer = getPlayerByIdx(nFanQianTarget);
	uint8_t nPerPlayer = PUNISH_COIN_BASE * (isBiXiaHu() ? 2 : 1);
	uint8_t nNeedAllCoin = nPerPlayer * (getSeatCnt() - 1);
	uint8_t nLingTou = 0;
	if (nNeedAllCoin > pLosePlayer->getChips())  // if not enough all , then kong fa to all
	{
		//nPerPlayer = pLosePlayer->getCoin() / 3;
		//nLingTou = pLosePlayer->getCoin() - nPerPlayer * 3;
		//nNeedAllCoin = pLosePlayer->getCoin();

		nPerPlayer = 0;
		nLingTou = 0;
		nNeedAllCoin = 0;
	}
	pLosePlayer->addSingleOffset(-1 * (int32_t)nNeedAllCoin);

	stSettle st;
	st.eSettleReason = (eMJActType)nSettleType;
	st.addLose(nFanQianTarget, nNeedAllCoin);
	// give winner 
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		uint8_t nRIdx = (nFanQianTarget + nIdx) % nSeatCnt;
		if (nRIdx == nFanQianTarget)
		{
			continue;
		}

		auto pWiner = getPlayerByIdx(nRIdx);
		uint16_t nWin = nPerPlayer;
		if (nLingTou > 0)
		{
			++nWin;
			--nLingTou;
		}
		pWiner->addSingleOffset(nWin);
		st.addWin(nRIdx, nWin);
	}

	addSettle(st);
}