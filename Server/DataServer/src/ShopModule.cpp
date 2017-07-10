#include "ShopModule.h"
#include "PlayerManager.h"
#include "GameServerApp.h"
#include "Player.h"
#include <sstream>
#include <ctime>
#include "AsyncRequestQuene.h"
#include "log4z.h"
#include "PlayerBaseData.h"
#include "MailModule.h"
void ShopModule::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
}

bool ShopModule::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID )
{
	switch ( nMsgType )
	{
	case MSG_SHOP_MAKE_ORDER:
	{
		auto nShopItemID = prealMsg["shopItemID"].asUInt();
		auto nPayChannel = prealMsg["channel"].asUInt();

		uint8_t nRet = 0;
		auto pItem = getShopItem(nShopItemID);
		CPlayer* pPlayer = nullptr;
		do
		{
			if ( nPayChannel != ePay_WeChat)
			{
				nRet = 3;
				break;
			}

			if (pItem == nullptr)
			{
				nRet = 1;
				break;
			}

			pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID( nTargetID );
			if (pPlayer == nullptr)
			{
				nRet = 2;
				break;
			}
		} while (0);

		if ( nRet )
		{
			prealMsg["ret"] = nRet;
			sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT );
			LOGFMTD("shopitem id = %d request order, failed ret = %u uid = %d", prealMsg["shopItemID"].asUInt(), nRet,nTargetID);
			return true;
		}

		Json::Value jsReq;
		jsReq["shopDesc"] = pItem->strDesc.c_str();
		jsReq["price"] = pItem->nPrice * 100;
		jsReq["channel"] = nPayChannel;
		jsReq["ip"] = pPlayer->getIp();

		std::ostringstream ss;
		ss << pItem->nShopID << "E" << pPlayer->getUserUID() << "E" << (uint32_t)time(nullptr);
		jsReq["outTradeNo"] = ss.str();

		auto pAsyncQue = getSvrApp()->getAsynReqQueue();
		pAsyncQue->pushAsyncRequest(ID_MSG_PORT_VERIFY, nTargetID, nTargetID, eAsync_Make_Order, jsReq, [this, nTargetID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData)
		{
			jsUserData["ret"] = 0;
			if (retContent["ret"].asUInt() != 0)
			{
				jsUserData["ret"] = 4;
				sendMsg(jsUserData, MSG_SHOP_MAKE_ORDER, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
				LOGFMTE("shopitem id = %d shop order failed, uid = %d", jsUserData["shopItemID"].asUInt(), nTargetID);
				return;
			}

			jsUserData["cPrepayId"] = retContent["cPrepayId"];
			jsUserData["cOutTradeNo"] = retContent["outTradeNo"];
			sendMsg(jsUserData, MSG_SHOP_MAKE_ORDER, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
			LOGFMTI("shopitem id = %d shop order ok, uid = %d", jsUserData["shopItemID"].asUInt(), nTargetID );
		}, prealMsg );

		LOGFMTD("shopitem id = %d request order, uid = %d", prealMsg["shopItemID"].asUInt(), nTargetID);
	}
	break;
	case MSG_SHOP_BUY_ITEM:
	{
		auto nShopItemID = prealMsg["shopItemID"].asUInt();
		auto nPayChannel = prealMsg["channel"].asUInt();
		auto cTranscation = prealMsg["transcationID"].asCString();

		uint8_t nRet = 0;
		auto pItem = getShopItem(nShopItemID);
		CPlayer* pPlayer = nullptr;
		do
		{
			if (nPayChannel != ePay_AppStore )
			{
				nRet = 4;
				break;
			}

			if (pItem == nullptr)
			{
				nRet = 1;
				break;
			}

			if ( strlen(cTranscation) != 20 )
			{
				nRet = 3;
				break;
			}

			pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 2;
				break;
			}
		} while (0);

		if ( nRet )
		{
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			jsRet["shopItemID"] = nShopItemID;
			sendMsg(jsRet, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
			LOGFMTD("shopitem id = %d buy item, failed ret = %u uid = %d", jsRet["shopItemID"].asUInt(), nRet, nTargetID);
			return true;
		}

		// go on verify 
		Json::Value jsReq;
		jsReq["shopItemID"] = nShopItemID;
		jsReq["price"] = pItem->nPrice;
		jsReq["channel"] = nPayChannel;
		jsReq["transcationID"] = cTranscation;

		auto pAsyncQue = getSvrApp()->getAsynReqQueue();
		pAsyncQue->pushAsyncRequest(ID_MSG_PORT_VERIFY, nTargetID, nTargetID, eAsync_Verify_Transcation, jsReq, [this, nTargetID, nShopItemID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData)
		{
			auto pItem = getShopItem(nShopItemID);
			bool isDelayGivedDiamond = false;

			// respone to current player 
			do
			{
				Json::Value jsBack;
				jsBack["ret"] = 0u;
				jsBack["shopItemID"] = nShopItemID;
				if (retContent["ret"].asUInt() != 0)
				{
					jsBack["ret"] = 5;
					sendMsg(jsBack, MSG_SHOP_BUY_ITEM, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
					LOGFMTE("shopitem id = %d buy item, uid = %d", nShopItemID, nTargetID);
					break;
				}

				// to add shop item to player 
				auto pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID(nTargetID);
				if (nullptr == pPlayer)
				{
					LOGFMTE("can not give coin to player player is nullptr = %u,shopitem id = %u", nTargetID, nShopItemID);
					isDelayGivedDiamond = true;
					break;
				}
				pPlayer->getBaseData()->modifyMoney(pItem->nDiamondCnt, true);
				sendMsg(jsBack, MSG_SHOP_BUY_ITEM, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
				LOGFMTI("shopitem id = %d buy ok, uid = %d", nShopItemID, nTargetID);
			} while (0);

			// do send mail ;
			auto pMail = ((DataServerApp*)getSvrApp())->getMailModule();
			Json::Value jsMailDetail;
			jsMailDetail["ret"] = retContent["ret"].asUInt() ;
			jsMailDetail["diamondCnt"] = pItem->nDiamondCnt;
			pMail->postMail(nTargetID, eMail_AppleStore_Pay, jsMailDetail, isDelayGivedDiamond ? eMailState_WaitSysAct : eMailState_SysProcessed);
		} );

		LOGFMTD("shopitem id = %d request verify, uid = %d", prealMsg["shopItemID"].asUInt(), nTargetID);
	}
	break;
	default:
		return false;
	}
	return true;
}

bool ShopModule::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	if ( eAsync_Recived_Verify_Result == nRequestType )
	{
		auto nShopItemID = jsReqContent["shopItemID"].asUInt();
		auto nRet = jsReqContent["ret"].asUInt();
		auto nTargetID = jsReqContent["targetID"].asUInt();

		auto pItem = getShopItem(nShopItemID);
		bool isDelayGivedDiamond = false;
		do
		{
			auto nChannel = jsReqContent["channel"].asUInt();
			Json::Value jsBack;
			jsBack["ret"] = 0u;
			jsBack["shopItemID"] = nShopItemID;
			if (nRet != 0)
			{
				jsBack["ret"] = 5;
				sendMsg(jsBack, MSG_SHOP_BUY_ITEM, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
				LOGFMTE("shopitem id = %d  recived verify result buy item, uid = %d", nShopItemID, nTargetID);
				break;
			}

			// to add shop item to player 
			auto pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (nullptr == pPlayer)
			{
				LOGFMTE("can not give coin to player player is nullptr = %u,shopitem id = %u", nTargetID, nShopItemID);
				isDelayGivedDiamond = true;
				break;
			}
			pPlayer->getBaseData()->modifyMoney(pItem->nDiamondCnt, true);
			sendMsg(jsBack, MSG_SHOP_BUY_ITEM, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
			LOGFMTI("shopitem id = %d recived verify result buy ok, uid = %d", nShopItemID, nTargetID);
		} while ( 0 );

		// do send mail ;
		auto pMail = ((DataServerApp*)getSvrApp())->getMailModule();
		Json::Value jsMailDetail;
		jsMailDetail["ret"] = nRet ;
		jsMailDetail["diamondCnt"] = pItem->nDiamondCnt;
		pMail->postMail(nTargetID, eMail_Wechat_Pay, jsMailDetail, isDelayGivedDiamond ? eMailState_WaitSysAct : eMailState_SysProcessed );
	}
	return false;
}

bool ShopModule::OnPaser(CReaderRow& refReaderRow)
{
	auto nShopItemID = refReaderRow["ShopItemID"]->IntValue();
	auto pItem = getShopItem(nShopItemID);
	if (pItem)
	{
		LOGFMTE( "already have item id = %u",nShopItemID );
		return true;
	}

	pItem = new stShopItem();
	pItem->nShopID = nShopItemID;
	pItem->nDiamondCnt = refReaderRow["diamondCnt"]->IntValue();
	pItem->nPrice = refReaderRow["price"]->IntValue();
	pItem->strDesc = refReaderRow["ShopItemName"]->StringValue();
	m_vShopItems.push_back(pItem);
	return true;
}

ShopModule::stShopItem* ShopModule::getShopItem(uint16_t nShopItemID)
{
	for (auto& ref : m_vShopItems)
	{
		if (ref->nShopID == nShopItemID)
		{
			return ref;
		}
	}
	return nullptr;
}