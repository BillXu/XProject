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
	uint32_t nEmojiCnt; 
};

struct stPlayerDetailData
	:public stPlayerBrifData
{
	double dfJ;  // jing du 
	double dfW;  // wei du ;
};

struct stServerBaseData
	:public stPlayerDetailData
{

};

#pragma pack(pop)