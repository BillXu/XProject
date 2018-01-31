#pragma once
#define MAX_HOLD_CARD_COUNT 13
#define HEAD_DAO_CARD_COUNT 3
#define OTHER_DAO_CARD_COUNT 5
enum eThirteenActType
{
	eThirteenAct_Call, //跟注
	eThirteenAct_AddCall, //加注
	eThirteenAct_PK, //比牌
	eThirteennAct_Look, //看牌
	eThirteenAct_CallToEnd, //一跟到底
	eThirteenAct_Pass, //弃牌
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
	DAO_HEAD = DAO_BEGIN,	//头道
	DAO_MIDDLE,				//中道
	DAO_TAIL,				//尾道
	DAO_MAX,
};

enum ThirteenType
{
	Thirteen_None,
	Thirteen_Single,//单张
	Thirteen_Double,//对子
	Thirteen_DoubleDouble,//两对
	Thirteen_ThreeCards,//三张
	Thirteen_Straight,//顺子
	Thirteen_Flush,//同花
	Thirteen_FuLu,//葫芦
	Thirteen_TieZhi,//铁枝
	Thirteen_StraightFlush,//同花顺
	Thirteen_5Tong,//5同
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
	WIN_SHUI_TYPE_NONE,//无
	WIN_SHUI_TYPE_SHOOT,//打枪
	WIN_SHUI_TYPE_SWAT,//全垒打
};

enum ROOM_OVER_TYPE
{
	ROOM_OVER_TYPE_TIME, //时间
	ROOM_OVER_TYPE_ROUND, //游戏局数
	ROOM_OVER_TYPE_MAX, //最大值
};