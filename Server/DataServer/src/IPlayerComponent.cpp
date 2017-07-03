#include "IPlayerComponent.h"
#include "Player.h"
IPlayerComponent::IPlayerComponent(CPlayer* pPlayer )
	:m_pPlayer(pPlayer),m_eType(ePlayerComponent_None)
{

}

IPlayerComponent::~IPlayerComponent()
{

}

bool IPlayerComponent::onMsg( stMsg* pMessage , eMsgPort eSenderPort)
{
	return false ;
}


void IPlayerComponent::sendMsg(stMsg* pbuffer , unsigned short nLen )
{
	m_pPlayer->sendMsgToClient(pbuffer,nLen );
}

void IPlayerComponent::sendMsg(Json::Value& jsMsg , uint16_t nMsgType  )
{
	m_pPlayer->sendMsgToClient(jsMsg,nMsgType );
}
