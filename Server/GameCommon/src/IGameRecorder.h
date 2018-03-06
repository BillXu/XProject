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
	std::shared_ptr<IPlayerRecorder> getPlayerRecorderInfo(uint32_t nUserID);
	void toJson(Json::Value& js);

protected:
	std::map<uint32_t, std::shared_ptr<IPlayerRecorder>> m_vPlayerRecorderInfo;
	uint32_t m_nFinishTime;
	uint32_t m_nReplayID;
	uint16_t m_nRoundIdx;
};

class IGameRoomRecorder
{
public:
	typedef std::map<uint32_t, uint32_t> MAP_UID_RECORD;
public:
	virtual ~IGameRoomRecorder(){}
	virtual void init(uint32_t nSieralNum, uint32_t nRoomID,uint32_t nRoomType,uint32_t nCreaterUID, Json::Value& jsOpts );
	void addSingleRoundRecorder(std::shared_ptr<ISingleRoundRecorder>& ptrSingleRecorder);
	std::shared_ptr<ISingleRoundRecorder> getSingleRoundRecorder(uint16_t nRoundUIdx);
	uint32_t getSieralNum();
	uint16_t getRoundRecorderCnt(); //返回下一局的idx
	uint16_t getRoundCnt(); //返回当前进行的局数
	void setPlayerCnt(uint32_t nCnt) { m_nPlayerCnt = nCnt; }
	void setClubID(uint32_t nClubID) { m_nClubID = nClubID; }
	void setLeagueID(uint32_t nLeagueID) { m_nLeagueID = nLeagueID; }
	void setRotBankerPool(uint32_t nCoin) { m_nRotBankerPool = nCoin; }
	void doSaveRoomRecorder( CAsyncRequestQuene* pSyncQuene  );
	void addDragIn(uint32_t nUserID, uint32_t nAmount);
	void getPlayerSingleRecorder(uint32_t nUserID, std::vector<std::shared_ptr<ISingleRoundRecorder>>& vRecorder);
protected:
	uint32_t m_nRoomID;
	uint32_t m_nRoomType;
	uint32_t m_nSieralNum;
	uint32_t m_nCreaterUID;
	uint32_t m_nPlayerCnt;
	uint32_t m_nClubID;
	uint32_t m_nLeagueID;
	uint32_t m_nRotBankerPool;
	uint16_t m_nCurRoundIdx;
	Json::Value m_jsOpts;
	std::map<uint16_t, std::shared_ptr<ISingleRoundRecorder>> m_vAllRoundRecorders;  // roundIdx : SingleRoundRecorder ;
	MAP_UID_RECORD m_mAllDragIn;
};