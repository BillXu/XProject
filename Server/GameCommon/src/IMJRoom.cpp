#include "IMJRoom.h"
#include "IGameRoomState.h"
#include <cassert>
#include "log4z.h"
#include "IMJPlayer.h"
#include "IMJPlayerCard.h"
#include "IPoker.h"
#include "IGameRoomManager.h"
#include "IMJPoker.h"
#include "MJReplayFrameType.h"

bool IMJRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr,nSeialNum,nRoomID,nSeatCnt,vJsOpts);
	m_pFanxingChecker = nullptr;
	setBankIdx(-1);
	return true;
}

bool IMJRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	GameRoom::onPlayerEnter(pEnterRoomPlayer);
	// check if already in room ;
	uint8_t nEmptyIdx = -1;
	for (uint8_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		if (getPlayerByIdx(nIdx) == nullptr)
		{
			nEmptyIdx = nIdx;
			break;
		}
	}

	if (nEmptyIdx == (uint8_t)-1)
	{
		LOGFMTE("why player enter , but do not have empty seat");
		return false;
	}

	doPlayerSitDown(pEnterRoomPlayer, nEmptyIdx );
	return true;
}

bool IMJRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if ( MSG_PLAYER_ACT == nMsgType )
	{
		auto actType = prealMsg["actType"].asUInt();
		auto p = getPlayerBySessionID(nSessionID);
		if (p == nullptr)
		{
			LOGFMTE("room id = %u why do act player is null ptr",getRoomID() );
			return true;
		}

		uint8_t nFrameType = -1;
		if (eMJAct_Pass == actType)
		{
			nFrameType = eMJFrame_Pass;
		}
		else if (eMJAct_Hu == actType)
		{
			nFrameType = eMJFrame_Hu;
		}
		else if (eMJAct_Peng == actType)
		{
			nFrameType = eMJFrame_Decl_Peng;
		}
		else if (eMJAct_MingGang == actType)
		{
			nFrameType = eMJFrame_Decl_MingGang;
		}
		else if (eMJAct_BuGang == actType || eMJAct_BuGang_Declare == actType || eMJAct_BuGang_Pre == actType)
		{
			nFrameType = eMJFrame_Decl_BuGang;
		}
		else if ( eMJAct_Ting == actType)
		{
			nFrameType = eMJFrame_Player_Ting;
		}
			 
		if ((uint8_t)-1 != nFrameType)
		{
			Json::Value jsFrameArg;
			jsFrameArg["idx"] = p->getIdx();
			addReplayFrame(nFrameType, jsFrameArg);
		}
	}

	 if ( MSG_SET_NEXT_CARD == nMsgType )
	{
#ifndef _DEBUG
		 return true;
#endif // !_DEBUG

		if (prealMsg["card"].isNull() || prealMsg["card"].isUInt() == false)
		{
			LOGFMTE( "MSG_SET_NEXT_CARD key is null or invalid" );
			return true;
		}
		getPoker()->pushCardToFron(prealMsg["card"].asUInt());
		return true ;
	}
	return GameRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
}

uint8_t IMJRoom::getBankerIdx()
{
	return m_nBankerIdx;
}

void IMJRoom::setBankIdx(uint8_t nIdx)
{
	m_nBankerIdx = nIdx;
}

void IMJRoom::onPlayerSetReady(uint8_t nIdx)
{
	auto pPlayer = getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("idx = %u target player is null ptr can not set ready",nIdx);
		return;
	}
	pPlayer->setState(eRoomPeer_Ready);
	// msg ;
	Json::Value jsMsg;
	jsMsg["idx"] = nIdx;
	sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_READY);
}

void IMJRoom::onStartGame()
{
	GameRoom::onStartGame();
	// distribute card 
	auto pPoker = getPoker();
	for (auto& pPlayer : m_vPlayers )
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}

		auto pMJPlayer = (IMJPlayer*)pPlayer;
		for (uint8_t nIdx = 0; nIdx < 13; ++nIdx)
		{
			auto nCard = pPoker->distributeOneCard();
			pMJPlayer->getPlayerCard()->addDistributeCard(nCard);
		}

		if (getBankerIdx() == pPlayer->getIdx())
		{
			auto nCard = pPoker->distributeOneCard();
			pMJPlayer->getPlayerCard()->onMoCard(nCard);
			pMJPlayer->signFlag(IMJPlayer::eMJActFlag_CanTianHu);
		}
		else
		{
			pMJPlayer->signFlag( IMJPlayer::eMJActFlag_WaitCheckTianTing );
		}
	}

	// prepare replay frame 
	Json::Value jsFrameArg, jsPlayers;
	jsFrameArg["bankIdx"] = getBankerIdx();
	for (auto& pPlayer : m_vPlayers)
	{
		if (!pPlayer)
		{
			LOGFMTE("why player is null hz mj must all player is not null");
			continue;
		}

		Json::Value jsPlayer;
		jsPlayer["idx"] = pPlayer->getIdx();

		IMJPlayerCard::VEC_CARD vCard;
		((IMJPlayer*)pPlayer)->getPlayerCard()->getHoldCard(vCard);
		Json::Value jsHoldCard;
		for (auto& vC : vCard)
		{
			jsHoldCard[jsHoldCard.size()] = vC;
		}

		jsPlayer["cards"] = jsHoldCard;
		jsPlayer["coin"] = pPlayer->getChips();
		jsPlayer["uid"] = pPlayer->getUserUID();
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}
	jsFrameArg["players"] = jsPlayers;
	addReplayFrame(eMJFrame_StartGame, jsFrameArg);
	LOGFMTI("room id = %u start game !",getRoomID());
}

bool IMJRoom::canStartGame()
{
	if ( !GameRoom::canStartGame() )
	{
		return false;
	}

	uint8_t nReadyCnt = 0;
	for (auto& pPlayer : m_vPlayers )
	{
		if (pPlayer && pPlayer->haveState(eRoomPeer_Ready) )
		{
			++nReadyCnt;
		}
	}
	return nReadyCnt == getSeatCnt();
}

// mj function ;
void IMJRoom::onWaitPlayerAct(uint8_t nIdx, bool& isCanPass)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("player idx = %u is null can not tell it wait act",nIdx);
		return;
	}
	auto pMJCard = pPlayer->getPlayerCard();
	// send msg to tell player do act 
	Json::Value jsArrayActs;
	Json::Value jsFrameActs;
	if ( isCanGoOnMoPai() )
	{
		// check bu gang .
		IMJPlayerCard::VEC_CARD vCards;
		pMJCard->getHoldCardThatCanBuGang(vCards);
		for (auto& ref : vCards)
		{
			Json::Value jsAct;
			jsAct["act"] = eMJAct_BuGang;
			jsAct["cardNum"] = ref;
			jsArrayActs[jsArrayActs.size()] = jsAct;
			jsFrameActs[jsFrameActs.size()] = eMJAct_BuGang;
		}
		// check an gang .
		vCards.clear();
		pMJCard->getHoldCardThatCanAnGang(vCards);
		for (auto& ref : vCards)
		{
			Json::Value jsAct;
			jsAct["act"] = eMJAct_AnGang;
			jsAct["cardNum"] = ref;
			jsArrayActs[jsArrayActs.size()] = jsAct;
			jsFrameActs[jsFrameActs.size()] = eMJAct_AnGang;
		}
	}

	// check hu .
	uint8_t nJiang = 0;
	if ( pMJCard->isHoldCardCanHu(nJiang))
	{
		Json::Value jsAct;
		jsAct["act"] = eMJAct_Hu;
		jsAct["cardNum"] = pMJCard->getNewestFetchedCard();
		jsArrayActs[jsArrayActs.size()] = jsAct;
		jsFrameActs[jsFrameActs.size()] = eMJAct_Hu;
	}

	isCanPass = jsArrayActs.empty() == false;
	jsFrameActs[jsFrameActs.size()] = eMJAct_Chu;

	// add default alwasy chu , infact need not add , becaust it alwasy in ,but compatable with current client ;
	Json::Value jsAct;
	jsAct["act"] = eMJAct_Chu;
	jsAct["cardNum"] = getAutoChuCardWhenWaitActTimeout(nIdx);
	jsArrayActs[jsArrayActs.size()] = jsAct;

	Json::Value jsMsg;
	jsMsg["acts"] = jsArrayActs;
	sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_AFTER_RECEIVED_CARD, pPlayer->getSessionID());
	
	if ( isCanPass )  // player do have option do select or need not give frame ;
	{
		Json::Value jsFrameArg;
		jsFrameArg["idx"] = nIdx;
		jsFrameArg["act"] = jsFrameActs;
		addReplayFrame( eMJFrame_WaitPlayerAct, jsFrameActs );
	}

	//LOGFMTD("tell player idx = %u do act size = %u",nIdx,jsArrayActs.size());
}

uint8_t IMJRoom::getAutoChuCardWhenWaitActTimeout(uint8_t nIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not chu card",nIdx);
		return 0;
	}
	return pPlayer->getPlayerCard()->getNewestFetchedCard();
}

uint8_t IMJRoom::getAutoChuCardWhenWaitChuTimeout(uint8_t nIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not chu card", nIdx);
		return 0;
	}
	IMJPlayerCard::VEC_CARD vCard;
	pPlayer->getPlayerCard()->getHoldCard(vCard);
	if (vCard.empty())
	{
		LOGFMTE("hold card can not be empty");
		assert(0&&"hold card must no be empty");
		return 0;
	}
	return vCard.back();
}

void IMJRoom::onPlayerMo(uint8_t nIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not mo", nIdx);
		return;
	}

	auto nNewCard = getPoker()->distributeOneCard();
	if (nNewCard == 0)
	{
		Assert(0,"invlid card" );
	}
	pPlayer->getPlayerCard()->onMoCard(nNewCard);
	pPlayer->zeroFlag();
	// send msg ;
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_Mo;
	msg["card"] = nNewCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["card"] = nNewCard;
	addReplayFrame(eMJFrame_Mo,jsFrameArg);
}

void IMJRoom::onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	auto pInvoker = (IMJPlayer*)getPlayerByIdx(nInvokeIdx);
	if (!pPlayer || !pInvoker)
	{
		LOGFMTE("why this player is null idx = %u , can not peng", nIdx);
		return;
	}

	if (pPlayer->getPlayerCard()->onPeng(nCard, nInvokeIdx ) == false)
	{
		LOGFMTE( "nidx = %u peng card = %u error",nIdx,nCard );
	}
	pInvoker->getPlayerCard()->onCardBeGangPengEat(nCard);
	pPlayer->zeroFlag();

	Json::Value jsmsg;
	jsmsg["idx"] = nIdx;
	jsmsg["actType"] = eMJAct_Peng;
	jsmsg["card"] = nCard;
	sendRoomMsg(jsmsg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["card"] = nCard;
	jsFrameArg["invokerIdx"] = nInvokeIdx;
	addReplayFrame(eMJFrame_Peng, jsFrameArg);
}

void IMJRoom::onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	auto pInvoker = (IMJPlayer*)getPlayerByIdx(nInvokeIdx);
	if (!pPlayer || !pInvoker )
	{
		LOGFMTE("why this player is null idx = %u , can not eat", nIdx);
		return;
	}

	if (pPlayer->getPlayerCard()->onEat(nCard,nWithA,nWithB) == false)
	{
		LOGFMTE("nidx = %u eat card = %u error, with a = %u ,b = %u", nIdx, nCard,nWithA,nWithB);
	}
	pInvoker->getPlayerCard()->onCardBeGangPengEat(nCard);

	// send msg ;
	Json::Value jsmsg;
	jsmsg["idx"] = nIdx;
	jsmsg["actType"] = eMJAct_Chi;
	jsmsg["card"] = nCard;
	Json::Value jseatwith;
	jseatwith[jseatwith.size()] = nWithA;
	jseatwith[jseatwith.size()] = nWithB;
	jsmsg["eatWith"] = jseatwith;
	sendRoomMsg(jsmsg, MSG_ROOM_ACT);

	// add replay frame
	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["card"] = nCard;
	jsFrameArg["invokerIdx"] = nInvokeIdx;
	jsFrameArg["eatWith"] = jseatwith;
	addReplayFrame(eMJFrame_Chi, jsFrameArg);
}

void IMJRoom::onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	auto pInvoker = (IMJPlayer*)getPlayerByIdx(nInvokeIdx);
	if (!pPlayer || !pInvoker)
	{
		LOGFMTE("why this player is null idx = %u , can not ming gang", nIdx);
		return;
	}
	pPlayer->signFlag(IMJPlayer::eMJActFlag_MingGang);
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_LouHu);
	pPlayer->addMingGangCnt();

	auto nGangGetCard = getPoker()->distributeOneCard();
	if (pPlayer->getPlayerCard()->onDirectGang(nCard, nGangGetCard,nInvokeIdx ) == false)
	{
		LOGFMTE("nidx = %u ming gang card = %u error,", nIdx, nCard );
	}
	pInvoker->getPlayerCard()->onCardBeGangPengEat(nCard);

	// send msg 
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_MingGang;
	msg["card"] = nCard;
	msg["gangCard"] = nGangGetCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["gang"] = nCard;
	jsFrameArg["newCard"] = nGangGetCard;
	jsFrameArg["invokerIdx"] = nInvokeIdx;
	addReplayFrame(eMJFrame_MingGang,jsFrameArg );
}

void IMJRoom::onPlayerAnGang(uint8_t nIdx, uint8_t nCard)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not an gang", nIdx);
		return;
	}
	pPlayer->signFlag(IMJPlayer::eMJActFlag_AnGang);
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_LouHu);
	pPlayer->addAnGangCnt();
	auto nGangGetCard = getPoker()->distributeOneCard();
	if (pPlayer->getPlayerCard()->onAnGang(nCard, nGangGetCard) == false)
	{
		LOGFMTE("nidx = %u an gang card = %u error,", nIdx, nCard);
	}

	// send msg ;
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_AnGang;
	msg["card"] = nCard;
	msg["gangCard"] = nGangGetCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["gang"] = nCard;
	jsFrameArg["newCard"] = nGangGetCard;
	addReplayFrame(eMJFrame_AnGang, jsFrameArg);
}

void IMJRoom::onPlayerBuGang(uint8_t nIdx, uint8_t nCard)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not bu gang", nIdx);
		return;
	}
	pPlayer->signFlag(IMJPlayer::eMJActFlag_BuGang);
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_LouHu);
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_DeclBuGang);

	auto nGangCard = getPoker()->distributeOneCard();
	if (pPlayer->getPlayerCard()->onBuGang(nCard, nGangCard) == false)
	{
		LOGFMTE("nidx = %u bu gang card = %u error,", nIdx, nCard);
	}
	pPlayer->addMingGangCnt();
	// send msg 
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_BuGang_Done;
	msg["card"] = nCard;
	msg["gangCard"] = nGangCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["gang"] = nCard;
	jsFrameArg["newCard"] = nGangCard;
	addReplayFrame(eMJFrame_BuGang, jsFrameArg);
}

void IMJRoom::onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)
{
	auto pInvoker = (IMJPlayer*)getPlayerByIdx(nInvokeIdx);
	if (!pInvoker)
	{
		LOGFMTE("room id = %u invoker idx = %u player is nullptr",getRoomID(),nInvokeIdx );
		return;
	}

	for (auto& ref : vHuIdx)
	{
		auto pPlayer = (IMJPlayer*)getPlayerByIdx(ref);
		if (!pPlayer)
		{
			LOGFMTE("why hu player is null room id = %u , idx = %u" , getRoomID(),ref );
			continue;
		}
		pPlayer->getPlayerCard()->onDoHu(nInvokeIdx, nCard, pInvoker->haveFlag(IMJPlayer::eMJActFlag_Gang));
		pPlayer->setState(eRoomPeer_AlreadyHu);
	
		// send msg 
		Json::Value msg;
		msg["idx"] = ref;
		msg["actType"] = eMJAct_Hu;
		msg["card"] = nCard;
		sendRoomMsg(msg, MSG_ROOM_ACT);
	}
}

void IMJRoom::onPlayerChu(uint8_t nIdx, uint8_t nCard)
{
	auto pPlayer = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (!pPlayer)
	{
		LOGFMTE("why this player is null idx = %u , can not chu", nIdx);
		return;
	}
	pPlayer->clearFlag(IMJPlayer::eMJActFlag_CanTianHu);

	// send msg ;
	Json::Value msg;
	msg["idx"] = nIdx;
	msg["actType"] = eMJAct_Chu;
	msg["card"] = nCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	if (!pPlayer->getPlayerCard()->onChuCard(nCard))
	{
		LOGFMTE("chu card error idx = %u , card = %u",nIdx,nCard );
	}

	Json::Value jsFrameArg;
	jsFrameArg["idx"] = nIdx;
	jsFrameArg["card"] = nCard;
	addReplayFrame( eMJFrame_Chu, jsFrameArg);
}

bool IMJRoom::isAnyPlayerPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard)
{
	for (auto& ref : m_vPlayers )
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx() )
		{
			continue;
		}

		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();
		if (pMJCard->canPengWithCard(nCard) )
		{
			return true;
		}

		if ( isCanGoOnMoPai() && pMJCard->canMingGangWithCard(nCard) ) // must can gang , will not run here , will return when check peng ;
		{
			return true;
		}

		if (((((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu) == false) && pMJCard->canHuWitCard(nCard)))
		{
			return true;
		}

		if (ref->getIdx() == (nInvokeIdx + 1) % getSeatCnt())
		{
			if (pMJCard->canEatCard(nCard))
			{
				return true;
			}
		}
	}

	return false;
}

void IMJRoom::onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vWaitHuIdx, std::vector<uint16_t>& vWaitPengGangIdx, bool& isNeedWaitEat)
{
	Json::Value jsFrameArg;

	for (auto& ref : m_vPlayers )
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		Json::Value jsMsg;
		jsMsg["invokerIdx"] = nInvokeIdx;
		jsMsg["cardNum"] = nCard;

		Json::Value jsActs;
		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();

		// check peng 
		if (pMJCard->canPengWithCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Peng;
			vWaitPengGangIdx.push_back(ref->getIdx());
		}

		// check ming gang 
		if ( isCanGoOnMoPai() && pMJCard->canMingGangWithCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_MingGang;
			// already add in peng ;  vWaitPengGangIdx
			if (vWaitPengGangIdx.empty())
			{
				vWaitPengGangIdx.push_back(ref->getIdx());
			}
		}

		if (ref->getIdx() == (nInvokeIdx + 1) % getSeatCnt())
		{
			isNeedWaitEat = false;
			if (pMJCard->canEatCard(nCard))
			{
				isNeedWaitEat = true;
				jsActs[jsActs.size()] = eMJAct_Chi;
			}
		}

		// check hu ;
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ( (isLouHu == false ) &&  pMJCard->canHuWitCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Hu;
			vWaitHuIdx.push_back(ref->getIdx());
		}

		if (jsActs.size() > 0)
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}

		if ( jsActs.size() == 0 )
		{
			continue;
		}
		jsMsg["acts"] = jsActs;
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, ref->getSessionID());

		//LOGFMTD("inform uid = %u act about other card room id = %u card = %u", ref->getUID(), getRoomID(),nCard );

		Json::Value jsFramePlayer;
		jsFramePlayer["idx"] = ref->getIdx();
		jsFramePlayer["acts"] = jsActs;

		jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitPlayerActAboutCard,jsFrameArg);
}

bool IMJRoom::isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)
{
	for (auto& ref : m_vPlayers )
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if (( isLouHu == false) && pMJCard->canHuWitCard(nCard))
		{
			return true;
		}
	}

	return false;
}

void IMJRoom::onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vCandinates)
{
	// send decalre gang msg ;
	Json::Value msg;
	msg["idx"] = nInvokeIdx;
	msg["actType"] = eMJAct_BuGang_Pre;
	msg["card"] = nCard;
	sendRoomMsg(msg, MSG_ROOM_ACT);

	// inform target player do this things 
	Json::Value jsFrameArg;
	for (auto& ref : m_vPlayers )
	{
		if (ref == nullptr || nInvokeIdx == ref->getIdx())
		{
			continue;
		}

		Json::Value jsMsg;
		jsMsg["invokerIdx"] = nInvokeIdx;
		jsMsg["cardNum"] = nCard;

		Json::Value jsActs;
		auto pMJCard = ((IMJPlayer*)ref)->getPlayerCard();
		// check hu 
		auto isLouHu = ((IMJPlayer*)ref)->haveFlag(IMJPlayer::eMJActFlag_LouHu);
		if ((isLouHu == false) && pMJCard->canHuWitCard(nCard))
		{
			jsActs[jsActs.size()] = eMJAct_Hu;
			vCandinates.push_back(ref->getIdx());

			jsFrameArg[jsFrameArg.size()] = ref->getIdx();
		}

		if ( jsActs.size() > 0 )
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}
		else
		{
			continue;
		}

		jsMsg["acts"] = jsActs;
		sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, ref->getSessionID());
		//LOGFMTD("inform uid = %u robot gang card = %u room id = %u ", ref->getUID(),nCard, getRoomID());
	}

	// add frame 
	addReplayFrame(eMJFrame_WaitRobotGang, jsFrameArg );
}

uint8_t IMJRoom::getNextActPlayerIdx(uint8_t nCurActIdx)
{
	return (nCurActIdx + 1) % getSeatCnt();
}

bool IMJRoom::isGameOver()
{
	return !isCanGoOnMoPai();
}

bool IMJRoom::isCanGoOnMoPai()
{
	return getPoker()->getLeftCardCount() > 0 ;
}

void IMJRoom::onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx)
{
	if ( isHaveLouHu() == false )
	{
		return;
	}

	auto p = (IMJPlayer*)getPlayerByIdx(nIdx);
	if ( p )
	{
		p->signFlag(IMJPlayer::eMJActFlag_LouHu);
	}
}

void IMJRoom::onPlayerLouPeng(uint8_t nIdx, uint32_t nLouCard)
{
	if (isHaveLouPeng() == false)
	{
		return;
	}

	auto p = (IMJPlayer*)getPlayerByIdx(nIdx);
	if (p == nullptr)
	{
		LOGFMTE( "why lou peng player idx = %u is nullptr card = %u",nIdx,nLouCard  );
		return;
	}
	p->getPlayerCard()->addLouPengedCard(nLouCard);
}

void IMJRoom::sendRoomInfo(uint32_t nSessionID)
{
	GameRoom::sendRoomInfo(nSessionID);
	// send hold card
}




