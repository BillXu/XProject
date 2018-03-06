#pragma once
#pragma pack(push)
#pragma pack(1)
#include "CommonDefine.h"
#include <vector>
#include <sstream>
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
	std::vector<uint32_t> vJoinedClub;
	std::vector<uint32_t> vCreatedClub;
	uint32_t nAllGame;
	uint32_t nWinGame;

	bool isJoinedClub(uint32_t nClubID) {
		return std::find(vJoinedClub.begin(), vJoinedClub.end(), nClubID) != vJoinedClub.end();
	}

	bool isCreatedClub(uint32_t nClubID) {
		return std::find(vCreatedClub.begin(), vCreatedClub.end(), nClubID) != vCreatedClub.end();
	}

	bool isInClub(uint32_t nClubID) {
		return isJoinedClub(nClubID) || isCreatedClub(nClubID);
	}

	std::string jcToString() {
		std::ostringstream ssSql;
		bool isFirst = true;
		for (auto ref : vJoinedClub) {
			if (isFirst) {
				isFirst = false;
				ssSql << std::to_string(ref);
			}
			else {
				ssSql << "." << std::to_string(ref);
			}
		}
		return ssSql.str();
	}

	std::string ccToString() {
		std::ostringstream ssSql;
		bool isFirst = true;
		for (auto ref : vCreatedClub) {
			if (isFirst) {
				isFirst = false;
				ssSql << std::to_string(ref);
			}
			else {
				ssSql << "." << std::to_string(ref);
			}
		}
		return ssSql.str();
	}
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
	void resetZero() {
		memset(cName, 0, sizeof(cName));
		memset(cHeadiconUrl, 0, sizeof(cHeadiconUrl));
		nUserUID = 0;
		nSex = 0;
		nCoin = 0;
		nDiamoned = 0;
		nEmojiCnt = 0;
		vJoinedClub.clear();
		vCreatedClub.clear();

		dfJ = 0;
		dfW = 0;
	}
};

#pragma pack(pop)