#pragma once
#include "IClubComponent.h"
class CClub 
{
public:
	CClub();
	~CClub();
	void init();
	void reset(); // for reuse the object ;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID);
	IClubComponent* getComponent(eClubComponentType eType) { return m_vAllComponents[eType]; }
	void onTimerSave();
	void onRelease();
	bool canRelease();

	void setName(const char* cName);
	void setIcon(const char* cIcon);
	void setClubID(uint32_t nClubID);
	void setCreatorUID(uint32_t nCreatorID);
	void setCreateTime();
	uint32_t getClubID();
	uint32_t getCreatorUID();

protected:
	stClubBaseData stBaseData;
	IClubComponent* m_vAllComponents[eClubComponent_Max];
};