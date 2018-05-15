#pragma once
#define MAX_LEN_CLUB_NAME 50
#define MAX_LEN_CLUBICON_URL 200
#define MAX_LEN_DESCRIPTION 200
#define MAX_LEN_REGION 50
#define AUTO_TREET_EVENT_TIME 24 * 60 * 60
#define AUTO_RELEASE_EVENT_TIME 48 * 60 * 60
#define DEFAULT_CLUB_MEMBER_LIMIT 200
#define DEFAULT_LEAGUE_MEMBER_LIMIT 50

enum eClubState
{
	eClubState_Delete,
	eClubState_Normal,
	eClubState_Max,
};

enum eClubEventState
{
	eClubEventState_Wait,	//待处理
	eClubEventState_Accede,	//请求通过
	eClubEventState_Decline,//请求拒绝
	eClubEventState_Max,
};

enum eClubComponentType
{
	eClubComponent_None,
	eClubComponent_MemberData,
	eClubComponent_GameData,
	eClubComponent_Event,            
	eClubComponent_Max,
};

enum eLeagueComponentType
{
	eLeagueComponent_None,
	eLeagueComponent_MemberData,
	eLeagueComponent_Event,
	eLeagueComponent_GameData,
	eLeagueComponent_Max,
};

enum eClubEventType
{
	eClubEventType_AppcationJoin, //申请加入俱乐部
	eClubEventType_AppcationEntry,//申请带入金币
	eClubEventType_GrantFoundation,//发放基金
	eClubEventType_ReciveIntegration,//接收积分
	eClubEventType_Dismiss, //申请解散俱乐部
	eClubEventType_FirePlayer, //申请踢出俱乐部玩家
	eClubEventType_PlayerQuit, //玩家退出
	eClubEventType_PlayerAddFoundation, //玩家给俱乐部充值基金
	eClubEventType_MTTAppcationEntry,//赛事报名重构(仅用于更新消息区分，区分方式为带入金币中详情的mtt值)
};

enum eLeagueEventType
{
	eLeagueEventType_AppcationJoin, //申请加入
	eLeagueEventType_FireClub, //踢出俱乐部
	eLeagueEventType_DismissLeague, //联盟解散
	eLeagueEventType_ClubQuit, //俱乐部退出
	eLeagueEventType_ClubEntry, //俱乐部带入积分变化
};

enum eClubMemberLevel
{
	eClubMemberLevel_None = 1, //初始会员等级
	eClubMemberLevel_Admin = 50, //管理员等级
	eClubMemberLevel_Creator = 100, //创建者设置上限100
};

enum eLeagueMemberLevel
{
	eLeagueMemberLevel_None = 1, //初始联盟成员等级
	eLeagueMemberLevel_Creator = 100, //创建者
};

enum eClubCreateRoomType
{
	eClubCreateRoom_All, //所有人，无限制
	eClubCreateRoom_Admin, //管理员以上级别
	eClubCreateRoom_Creator, //群主
};

enum eClubSearchLimit
{
	eClubSearchLimit_None, //无限制,所有人
	eClubSearchLimit_All, //限制所有人，不能被搜索到
};

enum eLeagueJoinLimit
{
	eLeagueJoinLimit_None, //无限制,所有人
	eLeagueJoinLimit_All, //限制所有人，不能搜索到
};

enum eClubUpdateLevelNeed
{
	eClubUpdateLevel_Icon = eClubMemberLevel_Admin,
	eClubUpdateLevel_Name = eClubMemberLevel_Admin,
	eClubUpdateLevel_CreateType = eClubMemberLevel_Admin,
	eClubUpdateLevel_SearchLimit = eClubMemberLevel_Admin,
	eClubUpdateLevel_Level = eClubMemberLevel_Creator,
	eClubUpdateLevel_Foundation = eClubMemberLevel_Admin,
	eClubUpdateLevel_Description = eClubMemberLevel_Admin,
	eClubUpdateLevel_GrantFoundation = eClubMemberLevel_Admin,
	eClubUpdateLevel_CreateLeague = eClubMemberLevel_Creator,
	eClubUpdateLevel_JoinLeague = eClubMemberLevel_Admin,
	eClubUpdateLevel_FireFromLeague = eClubMemberLevel_Creator,
	eClubUpdateLevel_DismissLeague = eClubMemberLevel_Creator,
	eClubUpdateLevel_QuitLeague = eClubMemberLevel_Creator,
	eClubUpdateLevel_MemberLimit = eClubMemberLevel_None,
	eClubUpdateLevel_MemberRemark = eClubMemberLevel_Creator,
};