#pragma once
#include "NativeTypes.h"
#include "json/json.h"
#include <memory>
class CAsyncRequestQuene;
class ISingleRoundRecorder
{
public:
	struct stPlayerRecorder
	{
		uint32_t nUserUID = 0 ;
		int32_t nOffset = 0 ;
		int32_t nWaiBaoOffset = 0 ;
	};
public:
	virtual ~ISingleRoundRecorder() {}
	virtual void init(uint16_t nRoundIdx, uint32_t nFinishTime, uint32_t nReplayID );
	uint16_t getRoundIdx();
	uint32_t getFinishTime();
	uint32_t getReplayID();
	bool addPlayerRecorderInfo( uint32_t nUserUID , int32_t nOffset , int32_t nWaitBaoOffset = 0 );
	void toJson(Json::Value& js );
	bool calculatePlayerTotalOffset(std::map<uint32_t, stPlayerRecorder>& vPlayers );
protected:
	std::map<uint32_t, stPlayerRecorder> m_vPlayerRecorderInfo;
	uint32_t m_nFinishTime;
	uint32_t m_nReplayID;
	uint16_t m_nRoundIdx;
};

class IGameRoomRecorder
{
public:
	virtual ~IGameRoomRecorder(){}
	virtual void init(uint32_t nSieralNum, uint32_t nRoomID,uint32_t nRoomType,uint32_t nCreaterUID, Json::Value& jsOpts );
	void addSingleRoundRecorder(std::shared_ptr<ISingleRoundRecorder>& ptrSingleRecorder);
	std::shared_ptr<ISingleRoundRecorder> getSingleRoundRecorder(uint16_t nRoundUIdx);
	uint32_t getSieralNum();
	uint16_t getRoundRecorderCnt();
	void doSaveRoomRecorder( CAsyncRequestQuene* pSyncQuene  );
protected:
	uint32_t m_nRoomID;
	uint32_t m_nRoomType;
	uint32_t m_nSieralNum;
	uint32_t m_nCreaterUID;
	Json::Value m_jsOpts;
	std::map<uint16_t, std::shared_ptr<ISingleRoundRecorder>> m_vAllRoundRecorders;  // roundIdx : SingleRoundRecorder ;
};