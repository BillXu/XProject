#pragma once
#include "json\json.h"
#include <list>
#include "Timer.h"
struct stEnterRoomData;
class IGameRoomManager;
class IGameRoom;
class RoomGroup
{
public:
	RoomGroup();
	~RoomGroup();
	bool init(IGameRoomManager* pmgr, Json::Value& jsCreateRoomOpts );
	void update( float fdeta );
	bool pushPlayerToEnterRoomQueue( stEnterRoomData& pPlayer );
	uint8_t checkEnterThisLevel( stEnterRoomData& pPlayer );
	uint32_t getLevel();
	IGameRoom* getRoomByID( uint32_t nRoomID );
	bool removePlayerFromQueue( uint32_t nUID );
	bool updatePlayerSessionID( uint32_t nUID , uint32_t nNewSessionID );
protected:
	IGameRoom* createRoom();
	bool doMatchPlayerEnterRoom();
protected:
	std::map<uint32_t, IGameRoom*> m_vAllRooms;
	std::vector<stEnterRoomData> m_vEnterRoomQuene;
	std::vector<stEnterRoomData> m_vEnterRoomRobotQuene;
	Json::Value m_jsRoomOpts;
	IGameRoomManager* m_pRoomMgr;
	static uint32_t s_MaxGroupRoomID;
	CTimer m_tMaxDelayMatchPlayer;
};