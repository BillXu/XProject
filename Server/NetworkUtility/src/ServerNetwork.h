#pragma once
#include "NetworkDefine.h"
#include <list>
class  IServerNetworkImp;
class CServerNetworkDelegate
{
public:
	CServerNetworkDelegate():m_nPriority(0){}
	virtual ~CServerNetworkDelegate(){}
	virtual bool OnMessage( Packet* pData ) = 0;
	virtual void OnNewPeerConnected( CONNECT_ID nNewPeer, ConnectInfo* IpInfo ){}
	virtual bool OnHeatBeat(CONNECT_ID nNewPeer) { return false; }
	virtual void OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo ){}
protected:
	unsigned int m_nPriority ;
};

class CServerNetwork
{
public:
	typedef std::list<CServerNetworkDelegate*> LIST_DELEGATE ;
	typedef bool (CServerNetwork::*lpFunc)(CServerNetworkDelegate* pDelegate, Packet* pData);
public:
	CServerNetwork();
	~CServerNetwork();
	bool StartupNetwork( unsigned short nPort , int nMaxInComming, bool isNative );
	void ShutDown();
	void RecieveMsg();
	void SendMsg(const char* pData , int nLength , CONNECT_ID& nSendToOrExcpet ,bool bBroadcast = false );
	void ClosePeerConnection(CONNECT_ID nPeerToClose);
	void AddDelegate(CServerNetworkDelegate* pDelegate , unsigned int nPriority = 0 );
	void RemoveDelegate(CServerNetworkDelegate* pDelegate );
protected:
	bool OnNewPeerConnected(CServerNetworkDelegate* pDelegate, Packet* pData  );
	bool OnPeerDisconnected(CServerNetworkDelegate* pDelegate, Packet* pData  );
	bool OnLogicMessage(CServerNetworkDelegate* pDelegate, Packet* pData);
	void EnumDelegate( lpFunc pFunc, Packet* pData );
protected:
	IServerNetworkImp* m_pNetPeer ;
	LIST_DELEGATE m_vAllDelegates ;
};