#pragma once
#pragma pack(push)
#pragma pack(1)
#include "CommonDefine.h"
#include <vector>
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
	uint8_t nTakeCharityTimes;
	uint32_t tLastTakeCardGiftTime;
	uint32_t nTotalDiamond;
	uint32_t nTotalGame;
	uint8_t nGateLevel;
	std::vector<uint32_t> vJoinedClubIDs;
	void reset()
	{
		vJoinedClubIDs.clear();
		dfJ = 0;
		dfW = 0;
		nUserUID = 0;
		memset(cName,0,sizeof(cName));
		memset(cHeadiconUrl, 0, sizeof(cHeadiconUrl));
	}
};

#pragma pack(pop)