#include "SeverInfo.h"
#include "log4z.h"
#include "CenterServer.h"
#include "ServerMessageDefine.h"
SeverInfo::~SeverInfo()
{
	m_tCheckWaitVerify.canncel();
	m_tCheckHeatBeat.canncel();
}

void SeverInfo::startWait()
{
	// start wait verify 
	m_tCheckWaitVerify.setInterval(1.5f);
	m_tCheckWaitVerify.setIsAutoRepeat(false);
	m_tCheckWaitVerify.setCallBack([this](CTimer* p, float f) { LOGFMTE("this port not verify wait timer out : %s",getIP() ); CCenterServerApp::SharedCenterServer()->closeConnection(getNetworkID()); });
	m_tCheckWaitVerify.start();

	// start check heat beat 
	m_tCheckHeatBeat.setInterval(TIME_HEAT_BEAT*2);
	m_tCheckHeatBeat.setIsAutoRepeat(true);
	m_tCheckHeatBeat.setCallBack([this](CTimer* p, float f)
	{ 
		if (m_isRecievedHeatBeat)
		{
			m_isRecievedHeatBeat = false;
			return;
		}
		LOGFMTE("heat beat time out ip = %s",getIP()); 
		CCenterServerApp::SharedCenterServer()->closeConnection(getNetworkID()); 
	}
	);
	m_tCheckHeatBeat.start();
}

void SeverInfo::onVerifiedSvr()
{
	m_tCheckWaitVerify.canncel();
	LOGFMTD("svr do verified ip = %s",getIP());
}

void SeverInfo::onReciveHeatBeat()
{
	m_isRecievedHeatBeat = true;
	//LOGFMTD("onReciveHeatBeat ip = %s", getIP());
}

// server group
void ServerGroup::init( eMsgPort nPortType, uint16_t nMaxSvrCnt )
{
	m_nPortType = nPortType;
	m_vSvrs.clear();
	m_vSvrs.resize(nMaxSvrCnt,nullptr);
}

ServerGroup::~ServerGroup()
{
	for (auto& ref : m_vSvrs)
	{
		if (ref)
		{
			delete ref;
			ref = nullptr;
		}
	}
	m_vSvrs.clear();
}

uint16_t ServerGroup::getSvrCnt()
{
	return (uint16_t)m_vSvrs.size();
}

bool ServerGroup::addServer( SeverInfo* pSvr, uint16_t nTargetIdx )
{
	if ((uint16_t)-1 != nTargetIdx)
	{
		if ( m_vSvrs.size() <= nTargetIdx )
		{
			LOGFMTE("reconnect svr idx = %u , is overflow , port = %s ip = %s",nTargetIdx,getPortDesc(),pSvr->getIP() );
			return false;
		}

		if ( m_vSvrs[nTargetIdx] != nullptr )
		{
			LOGFMTE("reconnect svr idx = %u , already have obj ip = %s , port = %s ip = %s", nTargetIdx, m_vSvrs[nTargetIdx]->getIP(), getPortDesc(), pSvr->getIP());
			return false;
		}
		pSvr->setInfo(nTargetIdx, pSvr->getNetworkID());
		m_vSvrs[nTargetIdx] = pSvr;
		LOGFMTI("svr reconnected port = %s, ip = %s idx = %u", getPortDesc(), pSvr->getIP(),nTargetIdx );
		return true;
	}

	for (uint8_t nIdx = 0; nIdx < m_vSvrs.size(); ++nIdx)
	{
		if (m_vSvrs[nIdx] == nullptr)
		{
			pSvr->setInfo(nIdx, pSvr->getNetworkID());
			m_vSvrs[nIdx] = pSvr;
			LOGFMTI("new svr connected port = %s, ip = %s", getPortDesc(),pSvr->getIP());
			return true;
		}
	}
	LOGFMTE("no more slot new svr connected port = %s, ip = %s", getPortDesc(), pSvr->getIP());
	return false;
}

bool ServerGroup::removeServer( CONNECT_ID nSvrConnect )
{
	for (uint8_t nIdx = 0; nIdx < m_vSvrs.size(); ++nIdx)
	{
		if (m_vSvrs[nIdx] && m_vSvrs[nIdx]->getNetworkID() == nSvrConnect )
		{
			LOGFMTE("svr disconnected port = %s, ip = %s", getPortDesc(), m_vSvrs[nIdx]->getIP());
			delete m_vSvrs[nIdx];
			m_vSvrs[nIdx] = nullptr;
			return true;
		}
	}
	return false;
}

SeverInfo* ServerGroup::getServerByConnectID( CONNECT_ID nSvrConnect )
{
	for (uint8_t nIdx = 0; nIdx < m_vSvrs.size(); ++nIdx)
	{
		if (m_vSvrs[nIdx] && m_vSvrs[nIdx]->getNetworkID() == nSvrConnect)
		{
			return m_vSvrs[nIdx];
		}
	}
	return nullptr;
}

CONNECT_ID ServerGroup::getNetworkIDByTargetID( uint32_t nMsgTargetID )
{
	if ( getSvrCnt() == 0 )
	{
		LOGFMTE("port = %s svr vec is null, connect transfer msg TargetID = %u", getPortDesc(),nMsgTargetID );
		return INVALID_CONNECT_ID;
	}

	uint16_t nIdx = nMsgTargetID % getSvrCnt();
	if ( m_vSvrs[nIdx] == nullptr)
	{
		LOGFMTE("port = %s idx = %d is nullptr can not send msg to it, targetId = %u", getPortDesc(),nIdx,nMsgTargetID );
		return INVALID_CONNECT_ID;
	}
	return m_vSvrs[nIdx]->getNetworkID();
}

void ServerGroup::getAllNetworkIDs(std::vector<CONNECT_ID>& vAllPortIDs)
{
	for (auto& ref : m_vSvrs)
	{
		if ( ref )
		{
			vAllPortIDs.push_back( ref->getNetworkID() );
		}
	}
}

eMsgPort ServerGroup::getPortType()
{
	return m_nPortType;
}

const char* ServerGroup::getPortDesc()
{
	return CCenterServerApp::getServerDescByType(getPortType());
}

bool ServerGroup::isHaveEmptySlot()
{
	for (uint8_t nIdx = 0; nIdx < m_vSvrs.size(); ++nIdx)
	{
		if ( m_vSvrs[nIdx] == nullptr )
		{
			return true;
		}
	}
	return false;
}