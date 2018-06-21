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
	void printInfo();

	void setBankerUID(uint32_t nBankerUID) { m_nBankerUID = nBankerUID; };
	void signRotBanker() { m_bRotBanker = true; }
	bool isRotBanker() { return m_bRotBanker; }
	uint32_t getBankerUID() { return m_nBankerUID; }

protected:
	std::map<uint32_t, std::shared_ptr<IPlayerRecorder>> m_vPlayerRecorderInfo;
	uint32_t m_nFinishTime;
	uint32_t m_nReplayID;
	uint16_t m_nRoundIdx;
	uint32_t m_nBankerUID = 0;
	bool m_bRotBanker = false;
};

class IGameRoomRecorder
{
public:
	struct reDraginInfo
	{
		uint32_t nAmount;
		uint32_t nClubID;
		uint32_t tOutTime = 0;
		uint32_t nOutIdx = 0;
	};
	typedef std::map<uint32_t, reDraginInfo> MAP_UID_RECORD;
	typedef std::pair<uint32_t, reDraginInfo> PAIR_UID_RECORD;
	struct URSortByOutTime {
		bool operator()(const PAIR_UID_RECORD& lhs, const PAIR_UID_RECORD& rhs) {
			if (lhs.second.tOutTime && rhs.second.tOutTime) {
				return lhs.second.tOutTime > rhs.second.tOutTime;
			}
			return lhs.second.tOutTime == 0 ? true : false;
		}
	};
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
	void setRotBankerPool(int32_t nCoin) { m_nRotBankerPool = nCoin; }
	void addRotBankerPool(uint32_t nUserID, uint32_t nAmount);
	void setDuration(int32_t nDuration) { m_nDuration = nDuration; }
	void doSaveRoomRecorder( CAsyncRequestQuene* pSyncQuene  );
	void addDragIn(uint32_t nUserID, uint32_t nAmount, uint32_t nClubID);
	void onMTTPlayerOut(uint32_t nUserID, uint32_t& tOutTime, uint32_t nOutIdx);
	void getPlayerSingleRecorder(uint32_t nUserID, std::vector<std::shared_ptr<ISingleRoundRecorder>>& vRecorder);
	uint32_t getMTTOutTime() { return ++m_nMTTOutTime; }
protected:
	uint32_t m_nRoomID;
	uint32_t m_nRoomType;
	uint32_t m_nSieralNum;
	uint32_t m_nCreaterUID;
	uint32_t m_nPlayerCnt;
	uint32_t m_nClubID;
	uint32_t m_nLeagueID;
	int32_t m_nRotBankerPool;
	uint16_t m_nCurRoundIdx;
	int32_t m_nDuration;
	Json::Value m_jsOpts;
	std::map<uint16_t, std::shared_ptr<ISingleRoundRecorder>> m_vAllRoundRecorders;  // roundIdx : SingleRoundRecorder ;
	MAP_UID_RECORD m_mAllDragIn;
	std::map<uint32_t, uint32_t> m_mRotBankerPool;
	
	uint32_t m_nMTTOutTime = 0;
};