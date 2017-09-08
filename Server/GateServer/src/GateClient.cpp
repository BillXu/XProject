#include "GateClient.h"
#include "GateServer.h"
#include "log4z.h"
#define TIME_WAIT_RECONNECT 360
stGateClient::stGateClient()
{
	m_lpfCloseCallBack = nullptr;
	m_lpfSendHeatBeatCallBack = nullptr;
	m_nSessionId = 0;
	m_nNetWorkID = INVALID_CONNECT_ID;
	m_isVerifyed = false;
	m_nUserUID = 0;
	m_strIPAddress = "undefined";

	m_isRecievedHeatBet = false;
	m_isWaitingReconnect = false;
}

stGateClient::~stGateClient()
{
	m_tCheckVerify.canncel();
	m_tCheckHeatBet.canncel();
	m_tWaitReconnect.canncel();
	m_tDelayClose.canncel();
}

void stGateClient::init(unsigned int nSessionID, CONNECT_ID nNetWorkID, const char* IpInfo)
{
	m_nSessionId = nSessionID;
	m_nNetWorkID = nNetWorkID;
	m_strIPAddress = IpInfo;
	m_isVerifyed = false;
	m_nUserUID = 0;
	m_isRecievedHeatBet = false;

	// start verify timer 
	m_tCheckVerify.setIsAutoRepeat(false);
	m_tCheckVerify.setInterval(1.0);
#ifdef _DEBUG
	m_tCheckVerify.setInterval(15.0);
#endif // DEBUG

	m_tCheckVerify.setCallBack([this](CTimer* p, float f) { if (false == isVerifyed()) { if (m_lpfCloseCallBack) { LOGFMTE("this gate is not verify , close it ip = %s",getIP()); m_lpfCloseCallBack(this,false); } } });
	m_tCheckVerify.start();

	// check heat beat 
	m_tCheckHeatBet.setIsAutoRepeat(true);
	m_tCheckHeatBet.setInterval( TIME_HEAT_BEAT * 2 );
	m_tCheckHeatBet.setCallBack([this](CTimer* p, float f) 
	{ 
		if ( false == m_isRecievedHeatBet) 
		{ 
			if (m_lpfCloseCallBack)
			{
				LOGFMTD("this gate is heat beat time out, close it ip = %s", getIP()); 
				m_lpfCloseCallBack(this, true); 
				return;
			}
		}
		m_isRecievedHeatBet = false;
	});

	m_tCheckHeatBet.start();
}

void stGateClient::onReconnected( stGateClient* pBeReconnected )
{
	m_nSessionId = pBeReconnected->getSessionID();
	m_nUserUID = pBeReconnected->getBindUID();
	LOGFMTD("str ip = %s do recconected session id = %u",getIP(),getSessionID() );
	pBeReconnected->beReconnected();
}

void stGateClient::bindUID(uint32_t nUID)
{
	m_nUserUID = nUID;
}

uint32_t stGateClient::getBindUID()
{
	return m_nUserUID;
}

uint32_t stGateClient::getSessionID()
{
	return m_nSessionId;
}

bool stGateClient::isVerifyed()
{
	return m_isVerifyed;
}

void stGateClient::doVerifyed()
{
	m_isVerifyed = true;
	m_tCheckVerify.canncel();
}

CONNECT_ID stGateClient::getNetworkID()
{
	return m_nNetWorkID;
}

const char* stGateClient::getIP()
{
	return m_strIPAddress.c_str();
}

void stGateClient::doRecivedHeatBet()
{
	m_isRecievedHeatBet = true;
}

void stGateClient::setCloseCallBack(pfnGateClientCallBack pFun)
{
	m_lpfCloseCallBack = pFun;
}

void stGateClient::setSendHeatBeatCallBack(pfnGateClientCallBack pFun)
{
	m_lpfSendHeatBeatCallBack = pFun;
}

void stGateClient::startWaitReconnect()
{
	if ( isWaitingReconnect() )
	{
		LOGFMTE("already waing reconected why wait again ? ip = %s",getIP());
		return;
	}

	m_tCheckHeatBet.canncel();

	if ( getBindUID() == 0 )
	{
		LOGFMTE("not logined in player direct close , not wait reconnect ip = %s",getIP());
		if (m_lpfCloseCallBack) 
		{
			m_lpfCloseCallBack(this, false);
		}
		return;
	}

	// wait reconnect  
	m_tWaitReconnect.canncel();
	m_tWaitReconnect.setIsAutoRepeat(false);
	m_tWaitReconnect.setInterval(TIME_WAIT_RECONNECT);
	m_tWaitReconnect.setCallBack([this](CTimer* p, float f) { if (m_lpfCloseCallBack) { m_lpfCloseCallBack(this, false); } });
	m_tWaitReconnect.start();
	m_isWaitingReconnect = true;
}

void stGateClient::beReconnected()
{
	m_tWaitReconnect.canncel();
	m_isWaitingReconnect = false;
	LOGFMTE("gate be reconnected , just closed it  ip = %s", getIP());
	return;
}

void stGateClient::reset()
{
	m_lpfCloseCallBack = nullptr;
	m_lpfSendHeatBeatCallBack = nullptr;
	m_nSessionId = 0 ;
	m_nNetWorkID = INVALID_CONNECT_ID;
	m_isVerifyed = false;
	m_nUserUID = 0 ;
	m_strIPAddress = "undefined";

	m_isRecievedHeatBet = false ;
	m_tCheckVerify.canncel() ;
	m_tCheckHeatBet.canncel();
	m_tWaitReconnect.canncel();
	m_tDelayClose.canncel();
	m_isWaitingReconnect = false;
}

void stGateClient::delayClose( float fDelay )
{
	if ( m_tDelayClose.isRunning() )
	{
		return;
	}
	LOGFMTD("start wait delay close");
	m_tDelayClose.setIsAutoRepeat(false);
	m_tDelayClose.setInterval(fDelay);
	m_tDelayClose.setCallBack([this](CTimer* p, float f) {   if (m_lpfCloseCallBack) { LOGFMTD("delay close client , close it ip = %s", getIP()); m_lpfCloseCallBack(this, false); } } );
	m_tDelayClose.start();
}