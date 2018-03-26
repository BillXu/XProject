#pragma once
#include "CommonDefine.h"
#define LOG_ARG_CNT 6
#define CROSS_SVR_REQ_ARG 4
#define RESEVER_GAME_SERVER_PLAYERS 100 
#define TIME_MATCH_PAUSE 60*30
#define MAX_NEW_PLAYER_HALO 120
#define  ROOM_CIRCLES_PER_VIP_ROOM_CARDS 4
#define ROOM_CARD_CNT_PER_CIRLE_NJMJ 2
enum eNetState
{
	eNet_Online,
	eNet_WaitReconnect,
	eNet_Offline,  // wait reconnect time out ;
	eNet_Max,
};

enum  eLogType
{
	eLog_Register, // externString, {ip:"ipdizhi"}
	eLog_Login,  // externString, {ip:"ipdizhi"}
	eLog_BindAccount, // externString, {ip:"ipdizhi"}
	eLog_Logout, 
	eLog_ModifyPwd, // externString, {ip:"ipdizhi"}
	eLog_TaxasGameResult, // nTargetID = roomid , vArg[0] = creator uid ,var[1] = public0 ---var[5] = public4, externString: {[ {uid:234,idx:2,betCoin:4456,card0:23,card1:23,offset:-32,state:GIVE_UP,coin:23334 },{ ... },{ ... }] } 
	eLog_AddMoney, // nTargetID = userUID , var[0] = isCoin , var[1] = addMoneyCnt, var[2] final coin, var[3] finalDiamond ,var[4] subType, var[5] subarg ;
	eLog_DeductionMoney,  // nTargetID = userUID , var[0] = isCoin , var[1] = DeductionCnt, var[2] final coin, var[3] finalDiamond, var[4] subType, var[5] subarg ;
	eLog_ResetPassword,
	eLog_NiuNiuGameResult, // nTargetID = room id , vArg[0] = bankerUID , vArg[1] = banker Times, vArg[2] = finalBottomBet, externString: {[ {uid:234,idx:2,betTimes:4456,card0:23,card1:23,card2:23,card3:23,card4:23,offset:-32,coin:23334 },{ ... },{ ... }] } 
	eLog_MatchResult, // nTargetID = room id , var[0] = room type ,var[1] = termNumber, var[2] room profit;
	eLog_PlayerSitDown, // nTargetID = playerUID , var[0] = room type , var[1] = roomID  , var[2] = coin;
	eLog_PlayerStandUp, // nTargetID = playerUID , var[0] = room type , var[1] = roomID  , var[2] = coin; 
	eLog_GetCharity,   // nTargetID = playerUID , var[0] = final coin ;
	eLog_PlayerLogOut, // nTargetID = playerUID , var[0] = final Coin ;
	eLog_Purchase, // nTargetID = playerUID , var[0] = final Coin ; var[1] = shop item id ;
	eLog_ExchangeOrder, // nTargetID = playerUID , var[0] exchange configID {playerName : "guest1145", excDesc : "this is fee card", remark : "my phone number is xxxxx" }
	eLog_RobotAddCoin, // nTargetID = robotUID , var[0] offset coin ; < 0  means save coin to banker, > 0 means add coin to robot ; 
	eLog_Max,
};

enum eLogDiamond
{
	eLogDiamond_Shop_AppStore,
	eLogDiamond_Shop_Wechat,
	eLogDiamond_Shop_Owner,
	eLogDiamond_Agent,
	eLogDiamond_Room,
	eLogCoin_Room,
	eLogDiamond_Max,
};


enum eAsyncReq 
{
	eAsync_DB_Select,   // { sql : "select * from table where uid = 345" , order : 0 } // order [ 0 - 3 ] biger first process ,  result : { afctRow : 1 , data : [row0,row1] }/// row { tile0 : value , title 0 ;}
	eAsync_DB_Update, // { sql : "select * from table where uid = 345" , order : 0 } // order [ 0 - 3 ] biger first process ,  result : { afctRow : 1 , data : [row0,row1] }/// row { tile0 : value , title 0 ;}
	eAsync_DB_Add,	// { sql : "select * from table where uid = 345" , order : 0 } // order [ 0 - 3 ] biger first process ,  result : { afctRow : 1 , data : [row0,row1] }/// row { tile0 : value , title 0 ;}
	eAsync_DB_Delete,	// { sql : "delete * from table where uid = 345" , order : 0 } // order [ 0 - 3 ] biger first process ,  result : { afctRow : 1 , data : [row0,row1] }/// row { tile0 : value , title 0 ;}
	eAsync_Player_Logined, 
	// { uid : 23, ip : "23.23.23.2" }
	eAsync_Make_Order, 
	// req: { shopDesc: "a golden" , outTradeNo : "234E232E23452" , price : 2342 , channel : 23 , ip : "23.234.234.23"  } 
	// result : { ret : 0 ,outTradeNo : "234E232E23452" , cPrepayId : "sdjfahsdigahsfg" , channel : 23  } 
	// outTradeNo =  [shopItemID]E[playerUID]E[utc time] 
	eAsync_Verify_Transcation,
	// req : { shopItemID : 23 , transcationID : "20 len" , channel : ePayChannel , price : 12  }
	// result : { ret : 0 }
	// ret : 0 success , 1 error ;
	eAsync_Recived_Verify_Result, // inform data svr player, { ret : 0 , targetID : 2345 , channel : ePaychannel , shopItemID : 23 } // result : null ;
	eAsync_AgentGetPlayerInfo, // { targetUID : 2345, agentID : 234 } , // ret { ret : 0 , isOnline : 0 , name : "hello name" , leftCardCnt : 2345  }  // ret : 1 , means success , 0 means can not find player ;
	eAsync_AgentAddRoomCard, // { targetUID : 234523, agentID : 234, addCard : 2345 , addCardNo  : 2345 }  // ret ; always success ; 
	eAsync_Request_EnterRoomInfo, //{ targetUID : 23, roomID : 23, sessionID : 23, port : 23 } , result : { ret : 0 ,uid : 2 , coin : 23 , diamond : 23,stayRoomID : 23  } ; // ret : 0 success , 1 already in other room , 2 session id error ;
	eAsync_Inform_Player_NetState, // { roomID : 23 , uid : 23 ,state : eNetState  } // result : null ; resut : { ret : 0 } ret : 0 ok , 1 can not find room id ;  
	eAsync_Inform_Player_NewSessionID, // { roomID : 23 , uid : 23 , newSessionID : 23 } // result : { ret : 0 } ret : 0 ok , 1 can not find room id ; 
	eAsync_Inform_Player_LeavedRoom, // { targetUID : 23 , roomID : 23, port : 23 }
	eAsync_Consume_Diamond, // { playerUID : 23 , diamond : 23 , roomID :23, reason : 0 }  // reason : 0 play in room , 1 create room  ;
	eAsync_GiveBackDiamond, // { targetUID : 2345 , diamond : 23,roomID : 23,reason : 0  } // reason : 0 play in room , 1 create room  ;
	eAsync_InformGate_PlayerLogout, //{ "sessionID" : 234 }
	eAsync_Request_CreateRoomInfo, // { targetUID : 23 , sessionID : 23 }  // result : { ret : 0 , uid  23 , diamond : 23 , alreadyRoomCnt : 23 }  // ret : 0 success , 1 session id error and uid not match, 2, not find target player 
	eAsync_Inform_CreatedRoom, // { targetUID : 23 , roomID : 234 } 
	eAsync_Inform_RoomDeleted, // { targetUID : 23 , roomID : 234  };
	eAsync_Check_WhiteList, // { listOwner: 234 , checkUID : 2 } , result : { ret : 0  } // 0 in white list , 1 not in white list ;

	eAsync_HttpCmd_SetCreateRoomFee,  // { isFree : 0 }  // result { ret : 0 , isFree : 0 }
	eAsync_HttpCmd_SetCanCreateRoom, // { canCreateRoom : 0 } // result { ret : 0 , canCreateRoom : 0 }
	eAsync_HttpCmd_GetSvrInfo, // result : { diffrent svr , diffrent key value } 
	eAsync_HttpCmd_DismissRoom, // { roomID : 22334 }, // result : { ret : 0 , roomID } , // when roomID == 1 means dismiss all ; 
	eAsync_HttpCmd_GetPlayerInfo, // { uid : 234 } , result :  { ret : 0 , uid : 2323, name: "abc" , diamond : 23 , emojiCnt : 23 , createRooms : [ { port : 2 , roomID : 23 }, ...] , stayInRoom: { port : 1 , roomID : 23 } }  
	eAsync_HttpCmd_AddEmojiCnt, // { targetUID : 234, addCnt : 23 , agentID : 23 }, result : { ret : 0, addCnt : 23 } // ret : 0 success , 1 argument error , 2 player is not online .

	eAsync_Comsume_Interact_Emoji, // { targetUID : 23 ,roomID : 23452, cnt : 1 } // result : { ret : 0 }  // 0 ok , 1 not enough, 2 player is not online ;
	eAsync_Comsume_Golden_Emoji, //

	eAsync_HttpCmd_GetClubLeagueInfo, // { clubID : 123 } // result { ret : 0, created : {123, 132, 312, 321...}, joined : {123, 321, 213, ...} }
	eAsync_HttpCmd_GetLeagueClubInfo, // { leagueID : 123 } // result { ret : 0, member : {123, 123, 123, ...} }
	eAsync_HttpCmd_GetClubLeagueIntegration, // { leagueID : 123, clubID : 123 } // {ret : 0, integration : 100, initialIntegration : 100}
	eAsync_HttpCmd_AddClubLeagueIntegration, // { leagueID : 123, clubID : 123, amount : 100 } // {ret : 0, integration : 100}
	eAsync_HttpCmd_AddClubLeagueIntialIntegration, // { leagueID : 123, clubID : 123, amount : 100 } // {ret : 0, initialIntegration : 100}
	eAsync_HttpCmd_StopClubInLeagueDragIn, // { leagueID : 123, clubID : 123, state : 0 } // { ret : 0, state : 1 }
	eAsync_HttpCmd_GetStopInfo, // { leagueID : 123, clubID : 123 } // { ret : 0, state : 1 }
	eAsync_HttpCmd_GetClubCreatFlag, // { clubID : 123 } // { ret : 0, state : 0 }
	eAsync_HttpCmd_SetClubCreatFlag, // { clubID : 123, state : 1 } // { ret : 0, state : 0 }


	//// above is new 
	//eAsync_CreateRoom, // extern MSG_CREATE_ROOM client , addtion : { roomID : 235, createUID : 3334, serialNum : 23455, chatRoomID : 2345234 }  // result : { ret : 0 } , must success ;
	//eAsync_DeleteRoom,// { roomID : 2345 }  // ret : { ret : 0 } // 0 success , 1 not find room , 2 room is running ;
	//eAsync_PostDlgNotice, // { dlgType : eNoticeType , targetUID : 2345 , arg : { ....strg } }
	//eAsync_OnRoomDeleted, // { roomID : 234 }

	//eAsync_ReqRoomSerials, // {roomType : 2 }  // result :  { ret : 0 , serials : [{ serial : 0 , chatRoomID : 2345} , { serial : 0 , chatRoomID : 2345} ,{ serial : 0 , chatRoomID : 2345} ] }  // ret : 0 success , 1 svr is reading from db wait a moment ; 
	eAsync_Apns, // { apnsType : 0 , targets : [234,2345,23,4] , content : "hello this is" ,msgID : "fs" ,msgdesc : "shfsg" }  apnsType : 0 , group type . 1 , target persions ;
	//
	//eAsync_ApplyLeaveRoom, // {uid : 234 , roomID : 2345 , reason : 0 } reason : 0 , disconnect , 1 other peer login.  result : { ret : 0 , coin : 2345 } // ret : 0 leave direct, 1 delay leave room , 2 not in room , 3 not find room   ;
	eAsync_player_game_add, //游戏局数变化
	eAsync_player_apply_DragIn,//玩家请求带入
	eAsync_player_apply_DragIn_Clubs,//玩家请求带入俱乐部列表
	eAsync_player_check_DragIn, //玩家带入金币检测
	eAsync_player_do_DragIn, //玩家带入金币
	eAsync_player_club_Push_Event,//玩家收到俱乐部推送请求
	eAsync_player_clubRoom_Back_Chip,//俱乐部房间结束时给玩家退回金币
	eAsync_player_apply_Show_Cards, //玩家请求明牌
	eAsync_player_do_Show_Cards, //玩家明牌
	eAsync_player_apply_Rot_Banker, //玩家请求抢庄
	eAsync_player_do_Rot_Banker, //玩家抢庄
	eAsync_player_League_Push_Event, //玩家收到联盟推送请求
	eAsync_player_club_decline_DragIn, //玩家收到俱乐部拒绝带入消息
	eAsync_player_DragInRoom_Closed, //玩家带入房间结束
	eAsync_thirteen_delay_check_Diamond, //十三水延时摆牌检测
	eAsync_thirteen_reput_check_Diamond, //十三水重新摆牌检测
	eAsync_Club_AddCoin, //俱乐部添加金币
	eAsync_Club_AddFoundation, //俱乐部充值基金
	eAsync_Club_Join, //申请通过加入俱乐部
	eAsync_Club_Create, //玩家创建俱乐部
	eAsync_Club_Dismiss, //玩家解散俱乐部
	eAsync_Club_Fire, //玩家被踢出
	eAsync_Club_Quit, //玩家退出
	eAsync_Club_CreateRoom_Check, //玩家创建俱乐部房间检查
	eAsync_Club_CreateRoom, //玩家创建俱乐部房间
	eAsync_club_apply_DragIn, //玩家向俱乐部申请带入金币
	eAsync_club_agree_DragIn, //玩家同意带入请求
	eAsync_club_decline_DragIn, //玩家拒绝带入请求
	eAsync_club_CreateRoom_Info, //玩家请求俱乐部牌局信息
	eAsync_club_League_Push_Event, //俱乐部收到联盟推送请求
	eAsync_club_Treat_Event_Message, //联盟处理事件发送群体消息
	eAsync_club_Update_Member_Limit_check_Diamond, //十三水增加人员上限钻石消耗验证
	eAsync_League_AddIntegration, //联盟发放积分
	eAsync_league_JoinLeague, //玩家申请加入联盟
	eAsync_league_ClubJoin, //俱乐部加入联盟
	eAsync_league_TreatEvent, //玩家请求处理联盟请求
	eAsync_league_FireClub_Check, //玩家请求处理联盟开除俱乐部资质审核
	eAsync_league_FireClub, //通知俱乐部被联盟开除
	eAsync_league_Dismiss_Check, //玩家请求解散联盟资质审核
	eAsync_league_Dismiss, //通知俱乐部们联盟已经解散
	eAsync_league_ClubQuit_Check, //玩家申请退出联盟资质审核
	eAsync_league_ClubQuit, //通知俱乐部已退出联盟
	eAsync_league_CreateRoom_Check, //俱乐部创建联盟房间检查
	eAsync_league_CreateRoom, //俱乐部创建联盟房间
	eAsync_league_CreateRoom_Info, //俱乐部申请联盟创建房间信息
	eAsync_league_or_club_DeleteRoom, //联盟或俱乐部删除房间信息
	eAsync_league_apply_DragIn_Clubs, //请求带入联盟房间可选择俱乐部列表
	eAsync_league_apply_Club_Integration, //俱乐部请求所在联盟积分
	eAsync_league_clubRoom_Back_Integration, //联盟局回退俱乐部积分
	eAsync_Max,
};


#define CHECK_MSG_SIZE(CHECK_MSG,nLen) \
{\
	if (sizeof(CHECK_MSG) > (nLen) ) \
{\
	LOGFMTE("Msg Size Unlegal msg") ;	\
	return false; \
	}\
	}

#define CHECK_MSG_SIZE_VOID(CHECK_MSG,nLen) \
{\
	if (sizeof(CHECK_MSG) > (nLen) ) \
{\
	LOGFMTE("Msg Size Unlegal msg") ;	\
	return; \
	}\
	}

#if defined(_DEBUG)
extern bool CustomAssertFunction(bool isfalse, char* description, int line, char*filepath);
#define Assert(exp, description) \
if( CustomAssertFunction( (int) (exp),description, __LINE__, __FILE__ )) \
{ _asm { int 3 } } 
#else
#define Assert( exp, description)
#endif