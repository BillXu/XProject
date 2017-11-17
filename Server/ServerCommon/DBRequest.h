#pragma once 
#ifndef Max_Sql_String
#define Max_Sql_String 1024*31 
#endif 
#include "MySqlData.h"
#include <vector>
#include <list>
#include <memory>
#include <functional>
enum eDBRequestType
{
	eRequestType_Add,
	eRequestType_Delete,
	eRequestType_Update,
	eRequestType_Select,
	eRequestType_Max,
};

enum eRequestOrder
{
	eReq_Order_Low,
	eReq_Order_Normal,
	eReq_Order_High,
	eReq_Order_Super,
	eReq_Order_Max,
};

struct stDBResult;
struct stDBRequest
{
public:
	typedef std::shared_ptr<stDBRequest> s_ptr;
	typedef std::function<void (stDBRequest::s_ptr pReq, std::shared_ptr<stDBResult> pRet)> lpDBResultCallBack;
public:
	eDBRequestType eType ;
	unsigned int nRequestUID ;  // maybe msg id  ;
	char pSqlBuffer[Max_Sql_String];
	int nSqlBufferLen ;
	void* pUserData ;
	lpDBResultCallBack lpfCallBack;
protected:
	stDBRequest() { reset(); }
	friend class DBRWModule;
	friend class CDBVerfiyTask;
public:
	void reset() { nRequestUID = 0; memset(pSqlBuffer, 0, sizeof(pSqlBuffer)); nSqlBufferLen = 0; pUserData = 0; lpfCallBack = nullptr; }
};

struct stDBResult
{
public:
	typedef std::shared_ptr<stDBResult> s_ptr;
public:
	typedef std::vector<CMysqlRow*> VEC_MYSQLROW ;
	~stDBResult();
	void reset();
public:
	stDBRequest::lpDBResultCallBack lpfCallBack;
	unsigned int nRequestUID ;
	VEC_MYSQLROW vResultRows ;  
	unsigned int nAffectRow ;
	void* pUserData ;
};
