#pragma once 
#include "IPlayerComponent.h"
#include "NativeTypes.h"
#include <string>
#include "CommonDefine.h"
#include <list>
class CPlayer;
struct stMail;
class CPlayerMailComponent
	:public IPlayerComponent
{
public:
	struct stMail
	{
		uint32_t nMailID;
		eMailType nMailType;
		uint32_t nPostTime;
		eMailState nState;
		Json::Value jsDetail;
	};
public:
	CPlayerMailComponent(CPlayer* pPlayer ):IPlayerComponent(pPlayer){}
	~CPlayerMailComponent();
	void onPlayerLogined()override;
	bool onRecievedMail( uint32_t nMailID, eMailType emailType,Json::Value& jsMailDetail ,uint32_t& nState, uint32_t nPostTime = 0 );
protected:
	bool doProcessMail( stMail* pMail );
	void readMail(uint16_t nOffset );
	void doProcessMailAfterReadDB();
	void updateMailsStateToDB( std::vector<uint32_t>& vMailIDs, eMailState eNewState );
protected:
	std::map<uint32_t,stMail*> m_vMails; // map default sort by key  ascendt , so erlier mail process first ; 
};