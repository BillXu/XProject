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
protected:
	std::shared_ptr<http::server::server> mHttpServer;
	std::map<std::string, httpHandle> vHttphandles;
};