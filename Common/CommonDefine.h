#pragma once
#include "NativeTypes.h"
#define MAX_LEN_ACCOUNT 40   // can not big then unsigned char max = 255
#define  MAX_LEN_PASSWORD 25 // can not big then unsigned char max = 255
#define MAX_LEN_CHARACTER_NAME 25 // can not big then unsigned char  max = 255
#define MAX_LEN_SIGURE 200   // can not big then unsigned char  max = 255
#define MAX_LEN_HEADICON_URL 200
#define MAX_LEN_EMAIL 50
#define MAX_LEN_SPEAK_WORDS 200  
#define MAX_MSG_BUFFER_LEN 59640

#ifdef _DEBUG
#define TIME_HEAT_BEAT 30  // heat beat time ;
#else
#define TIME_HEAT_BEAT 3  // heat beat time ;
#endif // _DEBUG

#define GOLDEN_PEER_CARD 3
#define MAX_ROOM_PEER 5
#define MAX_UPLOAD_PIC 4


#define NIUNIU_HOLD_CARD_COUNT 5

#define GOLDEN_PK_ROUND 2

#define MAX_IP_STRING_LEN 17

enum ePayChannel
{
	ePay_AppStore,
	ePay_WeChat,
	ePay_WeChat_365Niu = ePay_WeChat,
	ePay_ZhiFuBao,
	ePay_XiaoMi,
	ePay_WeChat_365Golden,
	ePay_Max,
};


enum eGameType
{
	eGame_None,
	eGame_NiuNiu,
	eGame_Max,
};

enum eRoomState
{
	// new state 
	eRoomSate_WaitReady,
	eRoomState_StartGame,
	eRoomState_Common_Max = 20,

	// niu niu special ;
	eRoomState_DecideBanker,
	eRoomState_RobotBanker = eRoomState_DecideBanker,
	eRoomState_DistributeFirstCard,
	eRoomState_DoBet ,
	eRoomState_DistributeCard, 
	eRoomState_DistributeFinalCard = eRoomState_DistributeCard,
	eRoomState_CaculateNiu,
	eRoomState_GameEnd,
	eRoomState_NN_Max = 50,
	
	// mj specail ;
	eRoomState_WaitPlayerAct,  // 等待玩家操作 { idx : 0 , huaCard : 23 }
	eRoomState_DoPlayerAct,  // 玩家操作 // { idx : 0 ,huIdxs : [1,3,2,], act : eMJAct_Chi , card : 23, invokeIdx : 23, eatWithA : 23 , eatWithB : 22 }
	eRoomState_AskForRobotGang, // 询问玩家抢杠胡， { invokeIdx : 2 , card : 23 }
	eRoomState_WaitPlayerChu, // 等待玩家出牌 { idx : 2 }
	eRoomState_MJ_Common_Max = 80, 

	// above is new ;
	eRoomState_None,
	eRoomState_WaitOpen,
	eRoomState_Opening,
	eRoomState_Pasue,
	eRoomState_Dead,
	eRoomState_WillDead,
	eRoomState_TP_Dead = eRoomState_Dead,
	eRoomState_WaitJoin,
	eRoomState_NN_WaitJoin = eRoomState_WaitJoin ,
	eRoomState_TP_WaitJoin = eRoomState_WaitJoin,
	eRoomState_WillClose,
	eRoomState_Close,
	eRoomState_DidGameOver,
	eRoomState_NN_Disribute4Card = eRoomState_StartGame,


	// state for golden
	eRoomState_Golden_Bet,
	eRoomState_Golden_PK,
	eRoomState_Golden_GameResult,

	// state for si chuan ma jiang 
	eRoomState_WaitExchangeCards, //  等待玩家换牌
	eRoomState_DoExchangeCards, // 玩家换牌
	eRoomState_WaitDecideQue,  // 等待玩家定缺
	eRoomState_DoDecideQue,  //  玩家定缺
	eRoomState_DoFetchCard, // 玩家摸牌
	
	
	
	eRoomState_WaitOtherPlayerAct,  // 等待玩家操作，有人出牌了 { invokerIdx : 0 , card : 0 ,cardFrom : eMJActType , arrNeedIdxs : [2,0,1] } 
	eRoomState_DoOtherPlayerAct,  // 其他玩家操作了。
	
	eRoomState_AskForHuAndPeng, // 询问玩家碰或者胡  { invokeIdx : 2 , card : 23 }
	eRoomState_WaitSupplyCoin, // 等待玩家补充金币  {nextState: 234 , transData : { ... } }
	eRoomState_WaitPlayerRecharge = eRoomState_WaitSupplyCoin,  //  等待玩家充值
	eRoomState_NJ_Auto_Buhua, // 南京麻将自动不花 
	
	eRoomState_Max,
};


enum eMJActType
{
	eMJAct_None,
	eMJAct_Mo = eMJAct_None, // 摸牌
	eMJAct_Chi, // 吃
	eMJAct_Peng,  // 碰牌
	eMJAct_MingGang,  // 明杠
	eMJAct_AnGang, // 暗杠
	eMJAct_BuGang,  // 补杠 
	eMJAct_BuGang_Pre, // 补杠第一阶段
	eMJAct_BuGang_Declare = eMJAct_BuGang_Pre, // 声称要补杠 
	eMJAct_BuGang_Done, //  补杠第二阶段，执行杠牌
	eMJAct_Hu,  //  胡牌
	eMJAct_Chu, // 出牌
	eMJAct_Pass, //  过 
	eMJAct_BuHua,  // 补花
	eMJAct_HuaGang, // 花杠
	eMJAct_Followed, // 连续跟了4张牌，要罚钱了
	eMJAct_4Feng, // 前4张出了4张不一样的风牌
	eMJAct_Ting,
	eMJAct_Max,
};

enum eFanxingType
{
	eFanxing_PingHu, // 平胡

	eFanxing_DuiDuiHu, //  对对胡

	eFanxing_QingYiSe, // 清一色
	eFanxing_QiDui, //  七对
	eFanxing_QuanQiuDuDiao, // 全球独钓
	eFanxing_TianHu, //天胡
	eFanxing_ShuangQiDui, // 双七对

	eFanxing_MengQing, // 门清
	
	eFanxing_YaJue, // 压绝 
	eFanxing_HunYiSe, // 混一色
	eFanxing_WuHuaGuo, // 无花果
	eFanxing_DiHu,//地胡

	eFanxing_HaiDiLaoYue, // 海底捞月
	eFanxing_DaMenQing, // 大门清
	eFanxing_XiaoMenQing, // 小门清

	eFanxing_Max, // 没有胡
};

enum eSettleType    // 这个枚举定义的只是一个中立的事件，对于发生事件的双方，叫法不一样，例如： 赢的人叫被点炮，输的人 叫 点炮。
{
	eSettle_DianPao,  // 点炮
	eSettle_MingGang, // 明杠
	eSettle_AnGang, //  暗杠
	eSettle_BuGang,  //  补杠
	eSettle_ZiMo,  // 自摸
	eSettle_HuaZhu,  //   查花猪
	eSettle_DaJiao,  //  查大叫
	eSettle_Max,
};

enum eTime
{
	eTime_WaitPlayerReady = 15,
	eTime_WaitRobotBanker = 5,
	eTime_ExeGameStart = 2,			// 执行游戏开始 的时间
	eTime_WaitChoseExchangeCard = 5, //  等待玩家选择换牌的时间
	eTime_DoExchangeCard = 3, //   执行换牌的时间
	eTime_WaitDecideQue = 10, // 等待玩家定缺
	eTime_DoDecideQue = 2, // 定缺时间
	eTime_WaitPlayerAct = 10,  // 等待玩家操作的时间
	eTime_WaitPlayerChoseAct = eTime_WaitPlayerAct,
	eTime_DoPlayerMoPai = 0,  //  玩家摸牌时间
	eTime_DoPlayerActChuPai = 1,  // 玩家出牌的时间
	eTime_DoPlayerAct_Gang = 0, // 玩家杠牌时间
	eTime_DoPlayerAct_Hu = 1,  // 玩家胡牌的时间
	eTime_DoPlayerAct_Peng = 0, // 玩家碰牌时间
	eTime_GameOver = 1, // 游戏结束状态持续时间
};


// ROOM TIME BY SECOND 
#define TIME_ROOM_WAIT_READY 5
#define TIME_ROOM_DISTRIBUTE 5
#define TIME_ROOM_WAIT_PEER_ACTION 30
#define TIME_ROOM_PK_DURATION 5
#define TIME_ROOM_SHOW_RESULT 5

// Golden room time 
#define TIME_GOLDEN_ROOM_WAIT_READY 10
#define TIME_GOLDEN_ROOM_DISTRIBUTY 3
#define TIME_GOLDEN_ROOM_WAIT_ACT 25
#define TIME_GOLDEN_ROOM_PK 4
#define TIME_GOLDEN_ROOM_RESULT 2

static unsigned char s_vChangeCardDimonedNeed[GOLDEN_PEER_CARD] = {0,4,8} ;


#define JS_KEY_MSG_TYPE "msgID"

// player State 
enum eRoomPeerState
{
	eRoomPeer_None,
	// peer state for taxas poker peer
	eRoomPeer_SitDown = 1,
	eRoomPeer_StandUp = 1 << 1,
	eRoomPeer_Ready =  (1<<12)|eRoomPeer_SitDown ,
	eRoomPeer_StayThisRound = ((1 << 2)|eRoomPeer_SitDown) ,
	eRoomPeer_WaitCaculate = ((1 << 7)|eRoomPeer_StayThisRound ),
	eRoomPeer_AllIn = ((1 << 3)|eRoomPeer_WaitCaculate) ,
	eRoomPeer_GiveUp = ((1 << 4)|eRoomPeer_StayThisRound),
	eRoomPeer_CanAct = ((1 << 5)|eRoomPeer_WaitCaculate),
	eRoomPeer_WaitNextGame = ((1 << 6)|eRoomPeer_SitDown ),
	eRoomPeer_WithdrawingCoin = (1 << 8),  // when invoke drawing coin , must be sitdown , but when staup up , maybe in drawingCoin state 
	eRoomPeer_LackOfCoin = (1<<9)|eRoomPeer_SitDown,
	eRoomPeer_WillLeave = (1<<10)|eRoomPeer_StandUp ,
	eRoomPeer_Looked =  (1<<13)|eRoomPeer_CanAct ,
	eRoomPeer_PK_Failed = (1<<14)|eRoomPeer_StayThisRound ,

	eRoomPeer_AlreadyHu = ((1 << 15) | eRoomPeer_CanAct),  //  已经胡牌的状态
	eRoomPeer_DelayLeave = (1 << 17),  //  牌局结束后才离开
	eRoomPeer_Max,
};


enum eSex
{
	eSex_Male,
	eSex_Female,
	eSex_Max,
};

enum eRoomPeerAction
{
	eRoomPeerAction_None,
	eRoomPeerAction_EnterRoom,
	eRoomPeerAction_Ready,
	eRoomPeerAction_Follow,
	eRoomPeerAction_Add,
	eRoomPeerAction_PK,
	eRoomPeerAction_GiveUp,
	eRoomPeerAction_ShowCard,
	eRoomPeerAction_ViewCard,
	eRoomPeerAction_TimesMoneyPk,
	eRoomPeerAction_LeaveRoom,
	eRoomPeerAction_Speak_Default,
	eRoomPeerAction_Speak_Text,
	// action for 
	eRoomPeerAction_Pass,
	eRoomPeerAction_AllIn,
	eRoomPeerAction_SitDown,
	eRoomPeerAction_StandUp,
	eRoomPeerAction_Max
};

enum eRoomFlag
{
	eRoomFlag_None ,
	eRoomFlag_ShowCard  ,
	eRoomFlag_TimesPK ,
	eRoomFlag_ChangeCard,
	eRoomFlag_Max,
};

 


// mail Module 
#define MAX_KEEP_MAIL_COUNT 50
enum eMailType
{
	eMail_Wechat_Pay, // { ret : 0 , diamondCnt : 23 }  // ret : 1 means verify error 
	eMail_AppleStore_Pay, // { ret : 0 , diamondCnt : 23 }   // ret : 1 means verify error 
	eMail_Agent_AddCard, // { agentID : 23 , serialNo : 2345 , cardOffset : 23 }
	eMail_Consume_Diamond, // { diamond : 23 , roomID :23, reason : 0 } 
	eMail_GiveBack_Diamond, // { diamond : 23 , roomID :23, reason : 0  } 

	// above is new ;
	eMail_SysOfflineEvent,// { event: concret type , arg:{ arg0: 0 , arg 1 = 3 } }  // processed in svr , will not send to client ;
	eMail_DlgNotice, // content will be send by , stMsgDlgNotice 
	eMail_Sys_End = 499,

	eMail_RealMail_Begin = 500, // will mail will show in golden server windown ;
	eMail_PlainText,  // need not parse , just display the content ;
	eMail_InvitePrize, // { targetUID : 2345 , addCoin : 300 } // you invite player to join game ,and give prize to you 
	eMail_Max,
};

enum eMailState
{
	eMailState_Unread,
	eMailState_WaitSysAct,
	eMailState_WaitPlayerAct,
	eMailState_SysProcessed,
	eMailState_Delete,
	eMailState_Max,
};

// poker timer measus by second
#define TIME_DISTRIBUTE_ONE_PUBLIC_CARD 0.5f

#define TIME_LOW_LIMIT_FOR_NORMAL_ROOM 10

// time for niuniu 
#define TIME_NIUNIU_DISTRIBUTE_4_CARD_PER_PLAYER 1.2f 
#define TIME_NIUNIU_TRY_BANKER 8.0f
#define TIME_NIUNIU_RAND_BANKER_PER_WILL_BANKER 0.7f
#define TIME_NIUNIU_PLAYER_BET 8.0f
#define TIME_NIUNIU_DISTRIBUTE_FINAL_CARD_PER_PLAYER 0.3f
#define TIME_NIUNIU_PLAYER_CACULATE_CARD 8.0f
#define TIME_NIUNIU_GAME_RESULT_PER_PLAYER 1.0f  //0.8f
#define TIME_NIUNIU_GAME_RESULT_EXT 8.0f

// time for golden
#define TIME_GOLDEN_DISTRIBUTE_CARD_PER_PLAYER 1.0f 



