#pragma once
#include <list>
#include "NativeTypes.h"
//#include "RakNetTypes.h"
#define MAX_VERIFY_STR_LEN 1024*8
#define MAX_MI_UID_LEN 30
enum eVerifiy_Result
{
	eVerify_Success,
	eVerify_Apple_Error,
	eVerify_Apple_Success,
	eVerify_DB_Error,
	eVerify_DB_Success,
	eVerify_Max,
};
struct stVerifyRequest
{
	char pBufferVerifyID[MAX_VERIFY_STR_LEN] = {0};  // base64 from cliend , or tranfaction_id from apple server ;
	uint8_t nChannel ; // ePayChannel ;
	eVerifiy_Result eResult ;  // eVerifiy_Result
	uint16_t nShopItemID ;  // for mutilp need to verify ;
	uint32_t nTargetID;   // buyer uid ;
	uint32_t nReqSieralNum;
	uint16_t nPrice;
	void* pUserData ;
};

typedef std::list<stVerifyRequest*> LIST_VERIFY_REQUEST ;

// order 
struct stShopItemOrderRequest
{
	char cShopDesc[50] ;
	char cOutTradeNo[32] ; // [shopItemID]E[playerUID]E[utc time] 
	uint32_t nPrize ; // fen wei dan wei ;
	char cTerminalIp[17] ;
	uint8_t nChannel ;
	char cPrepayId[64] ;
	uint8_t nRet ; // 0 success , 1 argument error ;

	uint32_t nTargetID ;
	uint32_t nReqSieralNum ;
	void* pUserData ;
};

typedef std::list<stShopItemOrderRequest*> LIST_ORDER_REQUEST ;