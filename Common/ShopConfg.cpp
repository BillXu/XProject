#include "ShopConfg.h"
#include "log4z.h"
#include "ConfigManager.h"
#include <time.h>

bool stShopItem::IsTimeLimitOk()
{
	if ( nEndTime == 0 && nBeginTime == 0 )
	{
		return true ; // no limit ;
	}
	time_t t = time(NULL) ;
	return t >= nBeginTime && t <= nEndTime ;
}

bool CShopConfigMgr::OnPaser(CReaderRow& refReaderRow )
{
	stShopItem* pItem = new stShopItem ;
	pItem->eType = (eShopItemType)refReaderRow["ShopItemType"]->IntValue() ;
	pItem->nShopItemID = refReaderRow["ShopItemID"]->IntValue() ;
	if ( GetShopItem(pItem->nShopItemID) )
	{
		delete pItem ;
		pItem = NULL ;
        #ifdef SERVER
		LOGFMTE("have two shop id the same") ;
#endif
		return false;
	}
#ifndef GAME_SERVER
	pItem->strItemName = refReaderRow["ShopItemName"]->StringValue() ;
#endif
	pItem->nFlag = refReaderRow["Flag"]->IntValue() ;
	pItem->nPrizeType = refReaderRow["PrizeType"]->IntValue() ;
	pItem->nOrigPrize = refReaderRow["OrigPrize"]->IntValue() ;
	pItem->nPrize = refReaderRow["CurPrize"]->IntValue() ;
	//pItem->nCanByTimes = refReaderRow["CanBuyTimes"]->IntValue();
	pItem->nItemID = refReaderRow["ItemID"]->IntValue();
	pItem->nCount = refReaderRow["Count"]->IntValue();
	m_vAllShopItems[pItem->nShopItemID] = pItem ;
	return true ;
}

stShopItem* CShopConfigMgr::GetShopItem(unsigned int nShopItemID )
{
	MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.find(nShopItemID) ;
	if ( iter != m_vAllShopItems.end() )
	{
		return iter->second ;
	}
	return NULL ;
}

unsigned int CShopConfigMgr::GetCurrencySize()
{
    int count = 0;
    return count;
}

unsigned int CShopConfigMgr::GetPropsSize()
{
    int count = 0;
    return count;
}

unsigned int CShopConfigMgr::GetAssetSize()
{
    int count = 0;
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (eShopItem_Assets == iter->second->eType) {
            count ++;
        }
    }
    return count;
}

unsigned int CShopConfigMgr::GetAppStoreProductSize()
{
    int count = 0;
#ifndef GAME_SERVER
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (0 == iter->second->nPrizeType) {// 0 RMB
            count ++;
        }
    }
#endif
    return count;
}


stShopItem* CShopConfigMgr::GetCurrencyByIndex(unsigned int index)
{
#ifndef SERVER
    int count = 0;
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    switch (index % 3) {
        case 0:
        {
            for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
            {
                if (eShopItem_Diamoned == iter->second->eType) {
                    if (count == index/3) {
                        return iter->second;
                    }
                    count ++;
                }
            }
        }
            break;
        case 1:
        {
            for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
            {
                if (eShopItem_Coin == iter->second->eType) {
                    if (count == index/3) {
                        return iter->second;
                    }
                    count ++;
                }
            }
        }
            break;
        case 2:
        {
			/* for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
			{
			if (eShopItem_UseItem == iter->second->eType) {
			CItemConfigManager* mgr = (CItemConfigManager *)CClientApp::SharedClientApp()->getConfigManager()->GetConfig(CConfigManager::eConfig_Item);
			stItemConfig* item = mgr->GetItemConfigByItemID(iter->second->nItemID);
			if (eItem_Gift == item->eType) {
			if (count == index/3) {
			return iter->second;
			}
			count ++;
			}
			}
			}*/
        }
            break;
            
        default:
            break;
    }
//    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
//    {
//        if (eShopItem_Diamoned == iter->second->eType || eShopItem_Coin == iter->second->eType) {
//            if (count == index) {
//                return iter->second;
//            }
//            count ++;
//        }
//    }
//    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
//    {
//        if (eShopItem_UseItem == iter->second->eType) {
//            CItemConfigManager* mgr = (CItemConfigManager *)CClientApp::SharedClientApp()->getConfigManager()->GetConfig(CConfigManager::eConfig_Item);
//            stItemConfig* item = mgr->GetItemConfigByItemID(iter->second->nItemID);
//            if (eItem_Gift == item->eType) {
//                if (count == index) {
//                    return iter->second;
//                }
//                count ++;
//            }
//        }
//    }
#endif
    return  NULL;
}

stShopItem* CShopConfigMgr::GetPropsByIndex(unsigned int index)
{

    return  NULL;
}

stShopItem* CShopConfigMgr::GetAssetByIndex(unsigned int index)
{
    int count = 0;
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (eShopItem_Assets == iter->second->eType) {
            if (count == index) {
                return iter->second;
            }
            count ++;
        }
    }
    return  NULL;
}

stShopItem* CShopConfigMgr::GetCoinProductByIndex(unsigned int index){
    int count = 0;
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (eShopItem_Coin == iter->second->nPrizeType) {
            if (count == index) {
                return iter->second;
            }
            count ++;
        }
    }
    return  NULL;
}
stShopItem* CShopConfigMgr::GetAppStoreProductByIndex(unsigned int index)
{
    int count = 0;
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (0 == iter->second->nPrizeType) {
            if (count == index) {
                return iter->second;
            }
            count ++;
        }
    }
    return  NULL;
}

stShopItem* CShopConfigMgr::GetAppStoreProductByProductID(std::string productID)
{
#ifndef GAME_SERVER
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (productID == iter->second->strAppStroeIdentifer) {
            return iter->second;
        }
    }
#endif
    return  NULL;
}

bool CShopConfigMgr::UpdateAppStoreProduct(std::string productID,std::string title,float price)
{
#ifndef SERVER
    MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin();
    for(iter=m_vAllShopItems.begin();iter!=m_vAllShopItems.end();++iter)
    {
        if (productID == iter->second->strAppStroeIdentifer) {
            iter->second->nPrize = price;
//            iter->second->strItemName = title;
            return true;
        }
    }
#endif
    return false;
}

void CShopConfigMgr::Clear()
{
	MAP_SHOP_ITEMS::iterator iter = m_vAllShopItems.begin() ; 
	for ( ; iter != m_vAllShopItems.end(); ++iter )
	{
		delete iter->second ;
		iter->second = NULL ;
	}
	m_vAllShopItems.clear() ;
}
