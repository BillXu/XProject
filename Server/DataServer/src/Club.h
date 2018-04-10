#pragma once
#include "json\json.h"
#include "MessageIdentifer.h"
#include "CommonDefine.h"
class ClubManager;
class Club
{
public:
	struct stMember
	{
		uint32_t nPlayerUID; 
		eClubPrivilige ePrivilige;   
	};

	struct stClubEvent
	{
		uint32_t nEventID;
		uint8_t nEventType; 
		Json::Value jsEventDetail;
		uint8_t nState;
		time_t nTime;
	};

	struct stInvitation
	{
		uint32_t nUserUID;
		time_t nTime;
	};

public:
	typedef std::map<uint32_t, stMember*> MAP_MEMBER;
	typedef std::map<uint32_t, stClubEvent*> MAP_EVENT;
public:
	~Club();
	bool init( ClubManager * pClubMgr, uint32_t nClubID, std::string& strClubName,Json::Value& jsCreateRoomOpts, uint32_t nCapacity, uint16_t nMaxMgrCnt, uint8_t nState = 0, uint32_t nDiamond = 0 , std::string strNotice = "");
	bool onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID );
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
	void onTimeSave();
	uint32_t getClubID();
	uint16_t getMgrCnt();
	uint32_t getCreatorUID();
	void onWillDismiss();
	void readClubDetail();

	void addMember(uint32_t nUserUID , eClubPrivilige ePrivilige = eClubPrivilige_Normal );
	void deleteMember( uint32_t nUserUID );
	bool isHaveMemeber( uint32_t nUserUID );
	stMember* getMember( uint32_t nUserUID );
	bool sendMsg(Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID, uint8_t nTargetPort = ID_MSG_PORT_CLIENT );
	void update(float fDeta);
	uint32_t getDiamond() { return m_nDiamond; }
	void updateDiamond(int32_t nDiamond);
	bool isPasuseState() { return 1 == m_nState; }
	std::string getName() { return m_strName; }
protected:
	void readClubEvents( uint32_t nAlreadyCnt );
	void readClubMemebers( uint32_t nAlreadyCnt );
	void saveEventToDB( uint32_t nEventID , bool isAdd ) ; // or update ?
	bool addEvent( stClubEvent* pEvent );
	void onCreateEmptyRoom( uint32_t nRoomID, int32_t nDiamondFee );
	void updateCreateRoom();
	void postMail( uint32_t nTargetID , eMailType eType , Json::Value& jsContent , eMailState eState );
	uint16_t getTargetPortByGameType( uint32_t nGameType );
protected:
	uint8_t m_nState;  // 0 normal , 1 pause ;
	uint32_t m_nCapacity;
	uint32_t m_nMaxEventID;
	uint16_t m_nMaxMgrCnt;
	uint32_t m_nClubID;
	uint32_t m_nDiamond;
	std::string m_strName;
	std::string m_strNotice;
	Json::Value m_jsCreateRoomOpts;
	bool m_isCreatingRoom; 
	MAP_MEMBER m_vMembers;
	MAP_EVENT m_vEvents;
	ClubManager* m_pMgr;
	std::vector<uint32_t> m_vFullRooms;
	std::vector<uint32_t> m_vEmptyRooms;
	std::vector<stInvitation*> m_vInvitations;

	bool m_isLackDiamond;
	float m_fDelayTryCreateRoom;
	bool m_isClubInfoDirty;
};








