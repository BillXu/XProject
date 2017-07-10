#pragma once
#include "IGlobalModule.h"
#include "IConfigFile.h"
class ShopModule
	:public IGlobalModule
	,public IConfigFile
{
public:
	struct stShopItem
	{
		uint16_t nShopID;
		uint16_t nPrice; // yuan 
		std::string strDesc;
		uint16_t nDiamondCnt;
	};
public:
	void init(IServerApp* svrApp)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
protected:
	bool OnPaser(CReaderRow& refReaderRow)override;
	stShopItem* getShopItem( uint16_t nShopItemID );
protected:
	std::vector<stShopItem*> m_vShopItems;
};
