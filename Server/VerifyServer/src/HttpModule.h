#pragma once 
#include "IGlobalModule.h"
#include "./HttpServer/server.hpp"
#include <functional>
#include <memory>
class CHttpModule
	:public IGlobalModule
{
public:
	typedef std::function<bool (http::server::connection_ptr)> httpHandle;
public:
	void init(IServerApp* svrApp)override;
	void update(float fDeta)override;
	bool registerHttpHandle(std::string strURI, httpHandle pHandle );
protected:
	bool onHandleVXPayResult(http::server::connection_ptr ptr );
	bool handleGetPlayerInfo(http::server::connection_ptr ptr );
	bool handleAddRoomCard(http::server::connection_ptr ptr);
	bool handleAnySdkLogin(http::server::connection_ptr ptr);

	// eg:
	// http cmd : POST mothed ;
	// post content : { cmd : 23 , targetID : 23 , arg : {} }
	// cmd : value from enum [ eAsyncReq ] ;
	// targetID : used caculate target svr idx ;
	// arg : a json object , diffient cmd , diffent value, representing  eAsyncReq request arg  ;

	// http cmd respone : a json object , diffient cmd , diffent value, also eAsyncReq result value ;
	bool handleHttpCmd(http::server::connection_ptr ptr);

	// used to get which svr port can process the cmd ( eAsyncReq )  ;
	uint16_t getSvrPortByCmd( uint16_t nCmd );
protected:
	std::shared_ptr<http::server::server> mHttpServer;
	std::map<std::string, httpHandle> vHttphandles;
};