#pragma once
#include "IGlobalModule.h"
#include "TaskPool.h"
#include "IVerifyTask.h"
struct stVerifyRequest ;
class CTaskPoolModule
	:public IGlobalModule
	,public ITaskFactory
{
public:
	enum eTask
	{
		eTask_WechatOrder,
		eTask_AppleVerify,
		eTask_WechatVerify,
		eTask_DBVerify,
		eTask_Apns,
		eTask_AnyLogin,
		eTask_Max,
	};
public:
	void init( IServerApp* svrApp )override ;
	void onExit()override ;
	void update(float fDeta )override ;
	bool onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)override;
	void testFunc();
	ITask::ITaskPrt createTask( uint32_t nTaskID )override ;
	CTaskPool& getPool(){ return m_tTaskPool ;}
	void doDBVerify( uint32_t nUserUID , uint16_t nShopID , uint8_t nChannel, std::string& strTransfcationID, uint32_t nFee );
	ITask::ITaskPrt getReuseTask(eTask nTask);
	void postTask(ITask::ITaskPrt pTask );
protected:
	void doDBVerify(IVerifyTask::VERIFY_REQUEST_ptr ptr);
	// logic 
	void onWechatOrder( uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nTargetID );
	void onVerifyMsg(uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nTargetID );
	void sendVerifyResult(std::shared_ptr<stVerifyRequest> & pResult );
protected:
	CTaskPool m_tTaskPool ;
};