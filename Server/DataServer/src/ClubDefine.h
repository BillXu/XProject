#pragma once
#define MAX_LEN_CLUB_NAME 50
#define MAX_LEN_CLUBICON_URL 200
#define MAX_LEN_DESCRIPTION 200
#define MAX_LEN_REGION 50

enum eClubState
{
	eClubState_Delete,
	eClubState_Normal,
	eClubState_Max,
};

struct stClubBaseData
{
	char cName[MAX_LEN_CLUB_NAME];
	char cHeadiconUrl[MAX_LEN_CLUBICON_URL];
	char cDescription[MAX_LEN_DESCRIPTION];
	char cRegion[MAX_LEN_REGION];
	uint32_t nClubID;
	uint32_t nCreatorUID;
	uint32_t nCreateTime;
	uint8_t nState;
	uint8_t nMemberLimit;
	uint32_t nFoundation;
	uint32_t nIntegration;
};