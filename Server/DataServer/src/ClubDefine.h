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
	eClubEventState_Wait,	//������
	eClubEventState_Accede,	//����ͨ��
	eClubEventState_Decline,//����ܾ�
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
	eClubEventType_AppcationJoin, //���������ֲ�
	eClubEventType_AppcationEntry,//���������
	eClubEventType_GrantFoundation,//���Ż���
	eClubEventType_ReciveIntegration,//���ջ���
	eClubEventType_Dismiss, //�����ɢ���ֲ�
	eClubEventType_FirePlayer, //�����߳����ֲ����
	eClubEventType_PlayerQuit, //����˳�
	eClubEventType_PlayerAddFoundation, //��Ҹ����ֲ���ֵ����
	eClubEventType_MTTAppcationEntry,//���±����ع�(�����ڸ�����Ϣ���֣����ַ�ʽΪ�������������mttֵ)
};

enum eLeagueEventType
{
	eLeagueEventType_AppcationJoin, //�������
	eLeagueEventType_FireClub, //�߳����ֲ�
	eLeagueEventType_DismissLeague, //���˽�ɢ
	eLeagueEventType_ClubQuit, //���ֲ��˳�
	eLeagueEventType_ClubEntry, //���ֲ�������ֱ仯
};

enum eClubMemberLevel
{
	eClubMemberLevel_None = 1, //��ʼ��Ա�ȼ�
	eClubMemberLevel_Admin = 50, //����Ա�ȼ�
	eClubMemberLevel_Creator = 100, //��������������100
};

enum eLeagueMemberLevel
{
	eLeagueMemberLevel_None = 1, //��ʼ���˳�Ա�ȼ�
	eLeagueMemberLevel_Creator = 100, //������
};

enum eClubCreateRoomType
{
	eClubCreateRoom_All, //�����ˣ�������
	eClubCreateRoom_Admin, //����Ա���ϼ���
	eClubCreateRoom_Creator, //Ⱥ��
};

enum eClubSearchLimit
{
	eClubSearchLimit_None, //������,������
	eClubSearchLimit_All, //���������ˣ����ܱ�������
};

enum eLeagueJoinLimit
{
	eLeagueJoinLimit_None, //������,������
	eLeagueJoinLimit_All, //���������ˣ�����������
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