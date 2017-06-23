#include "IGlobalModule.h"
#include "ISeverApp.h"
bool IGlobalModule::sendMsg(stMsg* pBuffer, uint16_t nLen, uint32_t nSenderUID)
{
	return getSvrApp()->sendMsg(pBuffer,nLen,nSenderUID);
}

bool IGlobalModule::sendMsg(Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID , uint8_t nTargetPort )
{
	return getSvrApp()->sendMsg(recvValue,nMsgID,nSenderUID,nTargetID,nTargetPort);
}