#include "IClubComponent.h"
#include "Club.h"

IClubComponent::IClubComponent()
{
	m_eType = eClubComponent_None;
}

IClubComponent::~IClubComponent()
{

}

void IClubComponent::init(CClub* pClub) {
	m_pClub = pClub;
}

void IClubComponent::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID)
{
	m_pClub->sendMsgToClient(jsMsg, nMsgType, nSessionID);
}