#pragma once
#include "NativeTypes.h"
#include "json/json.h"
#include <memory>
#include "ServerCommon.h"
// one round player recorder info ;
class IPlayerRecorder
{
public:
	IPlayerRecorder() { nUserUID = 0; nOffset = 0; }
	virtual ~IPlayerRecorder(){}
	virtual bool toJson(Json::Value& js) { Assert(nUserUID > 0 , "why player recorder uid = 0 ?" ); js["uid"] = getUserUID(); js["offset"] = getOffset(); return true; }
	uint32_t getUserUID() { return nUserUID; }
	int32_t getOffset() { return nOffset; }
	void setRecorder( uint32_t nUserUID , int32_t nOffset )
	{
		this->nUserUID = nUserUID;
		this->nOffset = nOffset;
	}
private:
	uint32_t nUserUID;
	int32_t nOffset;
};

class CAsyncRequestQuene;
class IGameRoomRecorder;
class ISingleRoundRecorder
{
public:
	virtual ~ISingleRoundRecorder() {}
	virtual void init(uint16_t nRoundIdx, uint32_t nFinishTime, uint32_t nReplayID );
	uint16_t getRoundIdx();
	uint32_t getFinishTime();
	uint32_t getReplayID();
	bool addPlayerRecorderInfo( std::shared_ptr<IPlayerRecorder> ptrPlayerRecorderInfo );
	void doSaveRoomRecorder(IGameRoomRecorder* pOwnRoomRecorder ,CAsyncRequestQuene* pSyncQuene, uint16_t nRoomType );
	bool calculatePlayerTotalOffset(std::map<uint32_t, int32_t>& vPlayersOffset );
protected:
	std::map<uint32_t, std::shared_ptr<IPlayerRecorder>> m_vPlayerRecorderInfo;
	uint32_t m_nFinishTime;
	uint32_t m_nReplayID;
	uint16_t m_nRoundIdx;
};

class IGameRoomRecorder
{
public:
	virtual ~IGameRoomRecorder(){}
	virtual void init(uint32_t nSieralNum, uint32_t nRoomID,uint32_t nRoomType,uint32_t nCreaterUID, uint32_t nClubID , Json::Value& jsOpts );
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
	uint32_t m_nClubID;
	Json::Value m_jsOpts;
	std::map<uint16_t, std::shared_ptr<ISingleRoundRecorder>> m_vAllRoundRecorders;  // roundIdx : SingleRoundRecorder ;
};