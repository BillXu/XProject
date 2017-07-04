#pragma once
#pragma pack(push)
#pragma pack(1)
#include "CommonDefine.h"
// base data about 
struct stPlayerBrifData
{
	char cName[MAX_LEN_CHARACTER_NAME];
	char cHeadiconUrl[MAX_LEN_HEADICON_URL];
	uint32_t nUserUID ;
	uint8_t nSex ; // eSex ;
	uint32_t nCoin ;
	uint32_t nDiamoned ;
};

struct stPlayerDetailData
	:public stPlayerBrifData
{
	//char cSignature[MAX_LEN_SIGURE] ;
	//double dfLongitude;
	//double dfLatidue;
	//uint32_t tOfflineTime ;  // last offline time ;
};

struct stServerBaseData
	:public stPlayerDetailData
{

};

#pragma pack(pop)