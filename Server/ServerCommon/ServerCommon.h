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
	eLogDiamond_Agent,
	eLogDiamond_Room,
	eLogDiamond_ClubAgent,
	eLogDiamond_ClubGiveBack,
	eLogDiamond_ClubConsume,
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
	eAsync_HttpCmd_AddClubDiamond, // { targetUID : 234, addCnt : 23 , agentID : 23 }, result : { ret : 0, addCnt : 23 } // ret : 0 success , 1 argument error , 2 club is not exsit .
	eAsync_HttpCmd_UpdateClubPointRestrict, // { clubID : 23 , isEanble : 0 } , result : { clubID : 23 , isEanble : 0 , ret : 0 }

	eAsync_Comsume_Interact_Emoji, // { targetUID : 23 ,roomID : 23452, cnt : 1 } // result : { ret : 0 }  // 0 ok , 1 not enough, 2 player is not online ;

	eAsync_ClubRoomGameOvered, // { clubID : 23 , roomID : 23 ,result : [ { uid : 23 , offset : -2 }, .... ] }
	eAsync_ClubGiveBackDiamond, // { clubID : 23 ,diamond : 23 }
	eAsync_ClubRoomStart, // { clubID : 23 , roomID : 23 }
	eAsync_ClubCreateRoom, // {  clubID : 23 , diamond : 234, roomIdx : 1  } extern creatOpts ,  // result : { ret : 0 , roomIdx : 1 ,roomID : 234, diamondFee : 23 } // ret:  1 ,diamond is not enough ,  2  admin stoped create room , 3  room ptr is null, 4 room id run out.  diamondFee : consume diamonds ;  
	eAsync_ClubCheckMember, // { clubID : 23 , uid : 234 } // result { ret : 0 } // ret : 0 ok , 1 not in club ;
	eAsync_ClubDismissRoom, // { roomID : 23 }, // result { roomID : 23 } 
	eAsync_HttpPost, // { url : "http://3sfhgss.com/a", postData : {} }  // result : { respon data }
	eAsync_ClubRoomONE,
	eAsync_HttpCmd_CreateClub, // { targetUID : 23, name = "aaa", opts : {json} } // { ret : 0, clubID : 123 }
	eAsync_HttpCmd_UpdateClubPlayerPT, // { clubID : 123, uid : 123, targetUID : 123, playTime : 0 } // { ret : 0 }
	eAsync_HttpCmd_ClubTreatEvent, // { clubID : 123, uid = 123, eventID : 123, detial : { isAgree : 1/0 } } // { ret : 0 }
	eAsync_HttpCmd_ClubKickPlayer, // { clubID : 123, uid = 123, kickUID : 123 } // { ret : 0 }
	eAsync_HttpCmd_CloseClub, // { clubID : 123, uid = 123, isPause = 1/0 } // { ret : 0 }
	eAsync_HttpCmd_UpdateClubCPRState, // { clubID : 123, uid = 123, state = 1/0 } // { ret : 0 }
	eAsync_GameOver, // one game end { playerUID = 123 }
	eAsync_HttpCmd_UpdateGateLevel, // { targetUID : 123, gateLevel : 1 } // { ret : 0 }
	eAsync_ClubCheckMemberLevel, // { clubID : 23 , uid : 234, port : 0 } // result { ret : 0, level : 1 }
	eAsync_HttpCmd_ApplyJoinClub, // { clubID : 23 , uid : 123 } // { ret : 0, clubID : 23 }
	eAsync_HttpCmd_bindAccount1, // { targetUID : 123, account : "123", password : "123"} svr : { ret : 0 } ret : 0 success , 1 account is occured , 2 uid is error , 5 : account or password error , 7 : timeout
	eAsync_HttpCmd_AgentAddPoint, // { targetUID : 123, addPoint : -123, addPointNo : 123} svr : { ret : 0 }
	eAsync_HttpCmd_ChangeVipLevel, // { targetUID : 123, level : 1, dayTime : 30 } svr : { ret : 0 }
	eAsync_ClubRoomSitDown, // { clubID : 23 , roomID : 23 }
	eAsync_ClubRoomStandUp, // { clubID : 23, roomID : 23 }
	eAsync_HttpCmd_UpdateClubAutoJoin, // { clubID : 123, uid = 123, state = 1/0 } // { ret : 0 }
	eAsync_HttpCmd_UpdateClubRoomOpts, // { clubID : 123, uid = 123, opts = { ... } } // { ret : 0 }
	eAsync_HttpCmd_TransferClubCreator, // { clubID : 123, targetUID = 123 } // { ret : 0 }
	eAsync_ClubRoomNetStateRefreshed, // { clubID : 23, roomID : 23, uid : 123, state : 0 }
	eAsync_PrivateRoomSitDown,
	eAsync_PrivateRoomStandUp,
	eAsync_PrivateRoomStart,
	eAsync_PrivateRoomGameOvered,
	eAsync_PrivateRoomNetStateRefreshed,
	eAsync_HttpCmd_ChangeClubVipLevel, // { clubID : 123, level : 1, dayTime : 30 } svr : { ret : 0 }

	//// above is new 
	//eAsync_CreateRoom, // extern MSG_CREATE_ROOM client , addtion : { roomID : 235, createUID : 3334, serialNum : 23455, chatRoomID : 2345234 }  // result : { ret : 0 } , must success ;
	//eAsync_DeleteRoom,// { roomID : 2345 }  // ret : { ret : 0 } // 0 success , 1 not find room , 2 room is running ;
	//eAsync_PostDlgNotice, // { dlgType : eNoticeType , targetUID : 2345 , arg : { ....strg } }
	//eAsync_OnRoomDeleted, // { roomID : 234 }

	//eAsync_ReqRoomSerials, // {roomType : 2 }  // result :  { ret : 0 , serials : [{ serial : 0 , chatRoomID : 2345} , { serial : 0 , chatRoomID : 2345} ,{ serial : 0 , chatRoomID : 2345} ] }  // ret : 0 success , 1 svr is reading from db wait a moment ; 
	eAsync_Apns, // { apnsType : 0 , targets : [234,2345,23,4] , content : "hello this is" ,msgID : "fs" ,msgdesc : "shfsg" }  apnsType : 0 , group type . 1 , target persions ;
	//
	//eAsync_ApplyLeaveRoom, // {uid : 234 , roomID : 2345 , reason : 0 } reason : 0 , disconnect , 1 other peer login.  result : { ret : 0 , coin : 2345 } // ret : 0 leave direct, 1 delay leave room , 2 not in room , 3 not find room   ;
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