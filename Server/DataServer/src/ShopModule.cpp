#include "ShopModule.h"
#include "PlayerManager.h"
#include "DataServerApp.h"
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
	LoadFile("../configFile/ShopConfig.txt");
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
		pAsyncQue->pushAsyncRequest(ID_MSG_PORT_VERIFY, nTargetID,eAsync_Make_Order, jsReq, [this, nTargetID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
		{
			if (isTimeOut)
			{
				jsUserData["ret"] = 4;
				sendMsg(jsUserData, MSG_SHOP_MAKE_ORDER, nTargetID, nTargetID, ID_MSG_PORT_CLIENT);
				LOGFMTE("request time out shopitem id = %d shop order failed, uid = %d", jsUserData["shopItemID"].asUInt(), nTargetID);
				return;
			}

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

			//if ( strlen(cTranscation) != 20 )
			//{
			//	nRet = 3;
			//	break;
			//}

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
		pAsyncQue->pushAsyncRequest(ID_MSG_PORT_VERIFY, nTargetID, eAsync_Verify_Transcation, jsReq, [this, nTargetID, nShopItemID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
		{
			uint32_t nDiamond = 0;
			uint8_t nRet = 0;
			do
			{
				if (isTimeOut)
				{
					nRet = 6;
					break;
				}
				nRet = retContent["ret"].asUInt();
				// respone to current player 
				if ( nRet )
				{
					nRet = 5;
					break;
				}

				auto pItem = getShopItem(nShopItemID);
				if (pItem == nullptr)
				{
					nRet = 7;
					break;
				}
				nDiamond = pItem->nDiamondCnt;
				LOGFMTI("shopitem id = %d buy ok, uid = %d", nShopItemID, nTargetID);
			} while ( 0 );

			auto pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer)
			{
				Json::Value jsBack;
				jsBack["ret"] = nRet;
				jsBack["shopItemID"] = nShopItemID;
				pPlayer->sendMsgToClient(jsBack, MSG_SHOP_BUY_ITEM);
			}

			// do send mail ;
			auto pMail = ((DataServerApp*)getSvrApp())->getMailModule();
			Json::Value jsMailDetail;
			jsMailDetail["ret"] = nRet;
			jsMailDetail["diamondCnt"] = nDiamond;
			pMail->postMail(nTargetID, eMail_AppleStore_Pay, jsMailDetail, eMailState_WaitSysAct );
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

		uint32_t nDiamond = 0;
		do
		{
			if ( nRet )
			{
				break;
			}

			auto pItem = getShopItem(nShopItemID);
			if (nullptr == pItem)
			{
				LOGFMTE("shopitem id = %d recived verify result buy ok, uid = %d item is null", nShopItemID, nTargetID);
				nRet = 7;
				break;
			}
			nDiamond = pItem->nDiamondCnt;
			LOGFMTI("shopitem id = %d recived verify result buy ok, uid = %d", nShopItemID, nTargetID);
		} while ( 0 );

		auto pPlayer = ((DataServerApp*)getSvrApp())->getPlayerMgr()->getPlayerByUserUID(nTargetID);
		if ( pPlayer )
		{
			Json::Value jsBack;
			jsBack["ret"] = 0u;
			jsBack["shopItemID"] = nShopItemID;
			if (nRet != 0)
			{
				jsBack["ret"] = 5;
			}
			pPlayer->sendMsgToClient(jsBack, MSG_SHOP_BUY_ITEM);
		}
		
		// do send mail ;
		auto pMail = ((DataServerApp*)getSvrApp())->getMailModule();
		Json::Value jsMailDetail;
		jsMailDetail["ret"] = nRet ;
		jsMailDetail["diamondCnt"] = nDiamond;
		pMail->postMail(nTargetID, eMail_Wechat_Pay, jsMailDetail, eMailState_WaitSysAct);
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