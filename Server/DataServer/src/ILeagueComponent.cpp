#include "ILeagueComponent.h"
#include "League.h"

ILeagueComponent::ILeagueComponent()
{
	m_eType = eLeagueComponent_None;
}

ILeagueComponent::~ILeagueComponent()
{

}

void ILeagueComponent::init(CLeague* pLeague) {
	m_pLeague = pLeague;
}

void ILeagueComponent::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID)
{
	m_pLeague->sendMsgToClient(jsMsg, nMsgType, nSessionID);
}