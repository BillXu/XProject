#pragma once
#define MAX_HOLD_CARD_COUNT 13
#define HEAD_DAO_CARD_COUNT 3
#define OTHER_DAO_CARD_COUNT 5
enum eThirteenActType
{
	eThirteenAct_Call, //��ע
	eThirteenAct_AddCall, //��ע
	eThirteenAct_PK, //����
	eThirteennAct_Look, //����
	eThirteenAct_CallToEnd, //һ������
	eThirteenAct_Pass, //����
};

enum eThirteenFrameType
{
	eThirteenFrame_StartGame,// { bankIdx : 2, players : [ { uid : 23,idx : 0, coin : 23 , cards : [1,3,4,5] }, .... ] }
	eThirteenFrame_WaitPlayerAct,  // { idx : 2, acts: [ act : eAct_Pass , info : 0 ] , [ act : 1 , info : 0 ] ... ] }
	eThirteenFrame_Pass, // { idx : 2 }
	eThirteenFrame_Call, // { idx : 0 , coin : 10 }
	eThirteenFrame_Look, // { idx : 0 }
	eThirteenFrame_PK, // { idx : 0 , withIdx : 1 , result : 1}
	eThirteenFrame_END_PK, // { participate : { 1 , 2 , 3 } , lose : { 1 , 2 } }
};

enum DAO_INDEX
{
	DAO_BEGIN,
	DAO_HEAD = DAO_BEGIN,	//ͷ��
	DAO_MIDDLE,				//�е�
	DAO_TAIL,				//β��
	DAO_MAX,
};

enum ThirteenType
{
	Thirteen_None,
	Thirteen_Single,//����
	Thirteen_Double,//����
	Thirteen_DoubleDouble,//����
	Thirteen_ThreeCards,//����
	Thirteen_Straight,//˳��
	Thirteen_Flush,//ͬ��
	Thirteen_FuLu,//��«
	Thirteen_TieZhi,//��֦
	Thirteen_StraightFlush,//ͬ��˳
	Thirteen_5Tong,//5ͬ
	Thirteen_Max,
};

enum PK_RESULT
{
	PK_RESULT_FAILED,
	PK_RESULT_WIN,
	PK_RESULT_EQUAL,
};

enum WIN_SHUI_TYPE
{
	WIN_SHUI_TYPE_NONE,//��
	WIN_SHUI_TYPE_SHOOT,//��ǹ
	WIN_SHUI_TYPE_SWAT,//ȫ�ݴ�
};

enum ROOM_OVER_TYPE
{
	ROOM_OVER_TYPE_TIME, //ʱ��
	ROOM_OVER_TYPE_ROUND, //��Ϸ����
	ROOM_OVER_TYPE_MAX, //���ֵ
};