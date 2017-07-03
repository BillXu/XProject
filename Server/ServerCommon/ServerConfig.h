#pragma once
#include "IConfigFile.h"
#include "ServerCommon.h"
#include "MessageIdentifer.h"
#include <vector>
struct stServerConfig
{
	unsigned char nSvrType ;
	char strIPAddress[261] ;
	unsigned short nPort ;
	char strAccount[30];
	char strPassword[30];
};

class CSeverConfigMgr
	:public IConfigFile
{
public: 
	typedef std::vector<stServerConfig> VEC_SERVER_CONFIG;
public:
	CSeverConfigMgr();
	~CSeverConfigMgr();
	virtual bool OnPaser(CReaderRow& refReaderRow ) ;
	stServerConfig* GetServerConfig(eMsgPort cSvrType, uint16_t nIdx = 0 ) ;
	stServerConfig* GetGateServerConfig(uint16_t nIdx ){ return GetServerConfig(ID_MSG_PORT_GATE,nIdx) ;}
	uint16_t GetServerConfigCnt(eMsgPort cSvrType ){ if (cSvrType >= ID_MSG_PORT_MAX) return NULL ; return m_vAllSvrConfig[cSvrType].size();}
protected:
	VEC_SERVER_CONFIG m_vAllSvrConfig[ID_MSG_PORT_MAX] ;
};