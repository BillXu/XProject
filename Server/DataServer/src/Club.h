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
		int32_t nOffsetPoints;
		int32_t nInitPoints;
		uint32_t nPlayTime;
//<<<<<<< HEAD
		std::string sRemark;
//=======
//>>>>>>> dev_haodi_MQMJ_SZMJ_NJMJ
		stMember() { nOffsetPoints = 0; nInitPoints = 0; nPlayTime = 0; }
		int32_t getCurPoint() { return nInitPoints + nOffsetPoints; }
		void decreasePlayTime() { nPlayTime -= 1; }
		void updatePlayTime(uint32_t tPlayTime) { nPlayTime = tPlayTime; }
		void updateRemark(std::string tRemark) { sRemark = tRemark; }
		void switchPlayTime()
		{
			if (nPlayTime == 1) {
				nPlayTime = 0;
			}
			else if (nPlayTime == 0) {
				nPlayTime = 1;
			}
			else {
				nPlayTime = 1;
			}
		}
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

	struct stClubRoomInfo
	{
		uint32_t nGameType;
		uint32_t nRoomID;
		uint32_t nRoomIdx;
		uint8_t nMaxPlayerCnt;
		uint8_t nCurPlayerCnt;
		bool bPrivate = false;
		void doPlayerSitDown() { nCurPlayerCnt++; }
		void doPlayerStandUp() { if (nCurPlayerCnt) { nCurPlayerCnt--; } }
		bool canEnter() { return nCurPlayerCnt < nMaxPlayerCnt; }
		uint8_t getDeficiency() { return  canEnter() ? nMaxPlayerCnt - nCurPlayerCnt : 0; }
	};

public:
	typedef std::map<uint32_t, stMember*> MAP_MEMBER;
	typedef std::map<uint32_t, stClubEvent*> MAP_EVENT;
public:
	~Club();
	bool init( ClubManager * pClubMgr, uint32_t nClubID, std::string& strClubName,Json::Value& jsCreateRoomOpts, uint32_t nCapacity, uint16_t nMaxMgrCnt, uint8_t nState = 0, uint8_t nCPRoomState = 0, uint8_t nAutoJoin = 0, uint8_t nVipLevel = 0, uint32_t tVipInvalidTime = 0, uint32_t nDiamond = 0 , std::string strNotice = "");
	bool onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID );
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
	void onTimeSave();
	void onSaveVipInfo();
	uint32_t getClubID();
	uint16_t getMgrCnt();
	uint32_t getCreatorUID();
	uint32_t getCreatorDiamond();
	bool isCreatorReady();
	bool modifyCreatorDiamond(int32_t nDiamond);
	void onWillDismiss();
	void readClubDetail();

	void addMember(uint32_t nUserUID , eClubPrivilige ePrivilige = eClubPrivilige_Normal );
	void deleteMember( uint32_t nUserUID );
	bool isHaveMemeber( uint32_t nUserUID );
	stMember* getMember( uint32_t nUserUID );
	bool sendMsg(Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID, uint8_t nTargetPort = ID_MSG_PORT_CLIENT );
	void update(float fDeta);
	uint32_t getDiamond() { return m_nDiamond; }
	bool checkDiamondEnough(uint32_t nOffset, uint32_t nRoomID);
	void updateDiamond(int32_t nDiamond, uint32_t nRoomID = 0);
	bool isPasuseState() { return 1 == m_nState; }
	std::string getName() { return m_strName; }
	bool canDismiss();
	void onFirstCreated()
	{
		m_isFinishReadEvent = true;
		m_isFinishReadMembers = true;
	}
	bool setIsEnablePointRestrict( bool isEnable );
	void decreaseMemberPlayTime(uint32_t nMemberUID);
	void updateMemberPlayTime(uint32_t nMemberUID, uint32_t nPlayTime);
	void updateMemberRemark(uint32_t nMemberUID, std::string sRemark);
	void switchMemberPlayTime(uint32_t nMemberUID);
	void clearLackDiamond(); //仅供外部调用，验证gateType
	uint8_t transferCreator(uint32_t nMemberUID);
	uint8_t getVipLevel() { return m_nVipLevel; }
	uint8_t changeVip(uint32_t nVipLevel, uint32_t nDay = 0);
protected:
	void readClubEvents( uint32_t nAlreadyCnt );
	void readClubMemebers( uint32_t nAlreadyCnt );
	void saveEventToDB( uint32_t nEventID , bool isAdd ) ; // or update ?
	bool addEvent( stClubEvent* pEvent );
	void onCreateEmptyRoom( uint32_t nIdx, uint32_t nGameType, uint32_t nRoomID,int32_t nDiamondFee , uint32_t nRoomIdx, uint8_t nSeatCnt, bool bPrivate = false );
	void updateCreateRoom();
	//uint8_t getEmptyAutoCreatRoomCnt();
	void postMail( uint32_t nTargetID , eMailType eType , Json::Value& jsContent , eMailState eState );
	uint16_t getTargetPortByGameType( uint32_t nGameType );
	void dismissEmptyRoom( uint32_t nIdx = 0, bool isWillDelteClub = false );
	bool isEnablePointsRestrict();

	// nLogType 0 牌局更新offsetPoint 。 detail { roomOffset: 23 , curPoint : 23 , roomID : 23 }
	// nLogType	1 管理重置offsetPoint 。detail { curPoint: 23 , mgrUID : 23 }
	//nLogType	2 管理员调整 initPoint。detail { initPoint: 23 , mgrUID : 23 }
	void savePointLog( uint32_t nPlayerUID , uint32_t nLogType , Json::Value& jsDetail );
	void saveMemberUpdateToDB( stMember * pMem );
	bool isInIRT(uint32_t nUserID);
	void joinIRT(uint32_t nUserID);
	void removeFromIRT(uint32_t nUserID);
	void sendIRT(Json::Value& jsMsg);
	void sortVipInfo();

	bool findCreateRoomOpts(uint32_t& nIdx, Json::Value& jsOpts);
	bool checkSameOpts(const Json::Value& jsOpts) { return false; }
	void clearEmptyRoomList();
	bool addRoomOpts(Json::Value jsOpts);
	bool eraseRoomOpts(uint32_t nOptsIdx);

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
	uint8_t m_nCreatePRoomState; // 0 can not, 1 can
	uint8_t m_nAutoJoin;
	uint8_t m_nVipLevel;
	uint32_t m_tVipInvalidTime;
	bool m_isCreatingRoom; 
	MAP_MEMBER m_vMembers;
	MAP_EVENT m_vEvents;
	ClubManager* m_pMgr;
	std::map<uint32_t, std::vector<stClubRoomInfo>> m_mFullRooms;
	std::map<uint32_t, std::vector<stClubRoomInfo>> m_mEmptyRooms;
	//std::vector<stClubRoomInfo> m_vFullRooms;
	//std::vector<stClubRoomInfo> m_vEmptyRooms;
	std::vector<stInvitation*> m_vInvitations;

	std::vector<uint32_t> m_vRealTimeInformation;

	bool m_isLackDiamond;
	float m_fDelayTryCreateRoom;
	bool m_isClubInfoDirty;
	bool m_bVipInfoDirty;
	uint32_t m_nMaxRoomIdx;

	bool m_isFinishReadEvent;
	bool m_isFinishReadMembers; 
};








