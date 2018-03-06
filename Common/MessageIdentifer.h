#pragma once 
enum eMsgPort
{
	ID_MSG_PORT_NONE, // client to game server 
	ID_MSG_PORT_CLIENT = ID_MSG_PORT_NONE,
	ID_MSG_PORT_GATE,
	ID_MSG_PORT_CENTER,
	ID_MSG_PORT_VERIFY,
	ID_MSG_PORT_RECORDER_DB,
	ID_MSG_PORT_DATA,
	ID_MSG_PORT_DB,
	ID_MSG_PORT_MJ,
	ID_MSG_PORT_BI_JI,
	ID_MSG_PORT_NIU_NIU,
	ID_MSG_PORT_DOU_DI_ZHU,
	ID_MSG_PORT_GOLDEN,
	ID_MSG_PORT_SCMJ,
	ID_MSG_PORT_THIRTEEN,
	ID_MSG_PORT_ALL_SERVER,
	ID_MSG_PORT_MAX,
};

enum eMsgType 
{
	MSG_NONE,
	//--new define begin---
	// the msg title used between servers 
	MSG_SERVERS_USE,
	MSG_VERIFY_SERVER,   // used between svr , not by transfer data ;
	MSG_VERIFY_CLIENT, // verify that is client ;
	MSG_TRANSER_DATA, // tranfer data between servers ;
	MSG_SERVER_DISCONNECT, // some svr disconnected ;  // svr recived , send by center svr , not by transfer data ;
	MSG_ASYNC_REQUEST, // asyn request 
	MSG_JSON_CONTENT,
	MSG_RECONNECT,   // client with gate 
	MSG_TELL_CLIENT_SVR_DISCONNECT, // tell client svr
	MSG_CLIENT_CONNECT_STATE_CHANGED,  // client connect state changed ;  // send by gate 
	MSG_GATE_SVR_IS_FULL, // gate connect is full , let client change other gate ;
	MSG_PLAYER_REGISTER,     // register an account ;
	MSG_PLAYER_LOGIN,  // check an account is valid ;
	MSG_TELL_GATE_PLAYER_OTHER_LOGIN,

	MSG_REQUEST_PLAYER_DATA, // request player brif data 
	// client : { nReqID : 23  isDetail : 0 }
	// svr : { uid : 23 , name : "hello" , headIcon : "http://weshg.wx.com",sex : 1 , ip : "1.0.0.1" , J : 23.0002, W : 232.234}  // J , W : GPS positon , maybe null ;
	MSG_PLAYER_OTHER_LOGIN,  // more than one place login , prelogin need disconnect ; client recived must disconnect from server
	// svr : null 
	MSG_PLAYER_BASE_DATA,
	// svr: { uid : 23,name : "lucy" , sex : 1 , headIcon : "http://url.com",diamond : 23 , coin : 20,emojiCnt : 23, ip: "234.234.234",stayRoomID : 0  }
	// stayRoomID , if not in any room , this key is null ;
	MSG_PLAYER_UPDATE_INFO,
	// client : { name : "lucy", sex : 1 , headIcon : "http://url.com"  }
	// svr: { ret : 0 }
	MSG_PLAYER_UPDATE_GPS,
	// client : { J : 23.23 , W : 2345  }   // j : jing du , w: wei du ;  
	// svr : null 
	MSG_PLAYER_REFRESH_MONEY,
	// client : null ;
	// svr { coin: 235 , diamond : 235 }
	MSG_PLAYER_LOGOUT,
	// client : null ;
	// svr : { ret : 0 }  // ret: 0 success , 1 can not find player , 2 player is doing something , can not logout,eg: in some room ;
	MSG_SHOP_MAKE_ORDER,
	// client : { shopItemID : 23 , channel : ePayChannel }  // ePayChannel 渠道枚举
	// svr : { ret : 0 , shopItemID : 23 , channel : ePayChannel,cPrepayId : "asdgsfh234g22jhbjadjg",cOutTradeNo : "232hlhsfhasdg" }
	// ret : 0 success , 1 can not find shop item , 2 , can not find player , 3 pay channel error , 4 argument error ; 
	MSG_SHOP_BUY_ITEM,
	// client : { shopItemID : 23 , channel : ePayChannel, transcationID : "20 len" }
	// svr : { ret : 0 , shopItemID : 23 }
	// ret : 0 success , 1 can not find shop item , 2 can not find player , 3 transcationID lenght is not 20 ,4 pay channel error ,5 platform verify error , 6 time out;

	MSG_ASYNC_REQUEST_RESULT,  // temp put here
	// in room msg ;
	MSG_REQUEST_ROOM_INFO,
	// client : null ;

	MSG_PLAYER_CHAT_MSG, // 玩家发送 聊天信息
	// client : { type : 1 , content : "biao qing or viceID" }
	// svr : { ret : 0 } 
	MSG_ROOM_CHAT_MSG, // 房间内有玩家 发送聊天信息；
	// svr:  { playerIdx : 2 , type : 1 , content : "biao qing or viceID" } 

	MSG_NEW_MAIL,
	// svr : { mailID : 234 , type : 0 ,state : 0 , detail : { } }
	// type : eMailType
	// state : eMailState 枚举值
	// detail : 不同的邮件类型，内容不一样；

	MSG_PLAYER_INTERACT_EMOJI,
	// client : { targetIdx : 0 , emoji : 23 }
	// svr : { ret : 0 }  // 0 success , 1 money is not enough , 2 player is not online , 3 you are not sitdown  in room ;
	
	MSG_ROOM_INTERACT_EMOJI,
	// svr : { invokerIdx : 1 ,targetIdx : 0 , emoji : 23 } 

	MSG_CREATE_ROOM = 300,
	// client: { uid : 234 ,gameType : 0 , seatCnt : 4 , payType : 1 , level : 2 , opts : {  .... }  }
	// payType : 0 the room owner pay cards , 1 AA pay card type , 2 big winer pay cards 
	// svr : {  ret : 0 , roomID : 23 } // ret : 0 success , 1 diamond is not enough, 2 create room count reach limit , 3 argument error , 4 room type error ,can not create room,  5  server maintenance,create room later   , 6 svr is busy , please try later , 7 internal time out;
	MSG_ROOM_REQ_ROOM_LIST, // send to data svr ;
	// client : null 
	// svr : { ret : 0 , rooms : [{id : 234, port : 23 }, ... ], stayInRoom : { id : 0 , port : 23 }  }
	// if player is not in any room , "stayInRoom" will be null , svr do not send this key 

	MSG_ENTER_ROOM,
	// client : { roomID : 23, uid : 23 }
	// svr: { roomID : 23 , ret : 0 } // ret : 0 success , 1 can not find room , 2 you already in other room ;3, room is full , 4, uid error ,5 , can not enter room , 6 unknown error, 7 arg not allow enter , 8 dianmond not enough;
	MSG_ROOM_CHANGE_STATE,
	// svr : { lastState : 23 , newState : 23 }
	MSG_ROOM_INFO, 
	// svr: { state : 2 , stateInfo : {} , roomID: 23, opts: {} , leftCircle : 2 ,isOpen : 1 , isWaitingDismiss : 0 ,applyDismissUID : 234, agreeIdxs : [2,3,1] ，leftWaitTime : 234, etc }
	// detail info , deponed on sepcail game ;

	MSG_ROOM_SIT_DOWN,   // tell all players , some one sit down ;
	// svr : { idx : 23 , uid : 23 ,isOnline : 0 , chips : 23 ... }  // the same as MSG_ROOM_INFO players item ;
	MSG_PLAYER_SIT_DOWN,
	// client : { idx : 1 } 
	// svr : { ret : 0 , idx : 1 } ;  // ret : 0 success , 1 pos already have players , 2, already in other room , 3, not in room , can not sit down , 4 already sit down , 5 session id error , 6 , unknown error  , 7 not in white list , 8 room open , can not sit down;
	MSG_PLAYER_STAND_UP,
	// client : null ;
	// svr : { ret : 0 } // ret : 0 success , 1 you are not sit down ,
	MSG_ROOM_STAND_UP,   // tell all players some one stand up 
	// svr : { idx : 23 , uid : 234 } 
	MSG_PLAYER_LEAVE_ROOM,
	// client : null 
	// svr : { ret : 0 }; // ret : 0 success , 1 you are not in this room , 2 unknown errro ;
	MSG_ROOM_REFRESH_NET_STATE, // player net state , changed 
	// svr : { idx : 0 , uid : 235 , state : 0  } , 0 is online , 1 lose connect , wait reconnect; 
	MSG_PLAYER_OPEN_ROOM,
	// client : null ;
	// svr: { ret : 0 } // 0 success , 1 you are not owner , 2 you are not in room ;
	MSG_ROOM_DO_OPEN,
	// svr : null ;
	MSG_ROOM_PLAYER_INFO,  // 房间内玩家的玩家信息，是一个数组,这个消息可能会收到多次
	// svr : {  players:[ { idx : 23 , uid : 23 ,isOnline : 0 , chips : 23, state : 23 ...}, ... ]  }

	MSG_APPLY_DISMISS_VIP_ROOM, // 申请解散vip 房间
	// client : { dstRoomID : 234 } 

	MSG_ROOM_APPLY_DISMISS_VIP_ROOM, //房间里有人申请解散vip 房间
	// svr : { applyerIdx : 2 }
	// applyerIdx : 申请者的idx 

	MSG_REPLY_DISSMISS_VIP_ROOM_APPLY,  // 答复申请解散的请求
	// client { dstRoomID : 23 , reply : 0 }
	// reply ： 1 表示同意， 0 表示拒绝。

	MSG_ROOM_REPLY_DISSMISS_VIP_ROOM_APPLY, // 收到有人回复解散房间
	// svr { idx : 23 , reply : 0 }
	// reply ： 1 表示同意， 0 表示拒绝。

	MSG_VIP_ROOM_DO_CLOSED, // vip 房间结束通知
	// svr : { isDismiss : 0 , roomID : 2345 , eType : eroomType }  
	// isDismiss : 是否是解散结束的房间。1 是解散房间，0 是自然结束。

	MSG_ADD_WHILE_LIST,
	// client : { uid : 23 }
	// ret : { ret : 0 , uid : 23 }
	// 0 success , 1 white list is full  ;

	MSG_REMOVE_WHITE_LIST,
	// client : { uid : 23 }
	// svr : { ret : 0 , uid : 23 }
	// ret : 0 success , 1 , uid not in white list ;

	MSG_REQUEST_WHITE_LIST,
	// client : null
	// svr : { list : [ 23, 232,52 ... ] } 

	MSG_PLAYER_SET_READY = 600,   	// player do ready
	// client : { dstRoomID : 2345 } ;
	// svr : { ret : 1 , curState : 23 } // 1 you are not in room , 2 you are not state waitNextGame, tell curState ;
	MSG_ROOM_PLAYER_READY,  // some player did ready
	// svr : { idx : 2 }

	// niuniu msg 
	MSG_ROOM_PRODUCED_BANKER,
	// svr : { bankerIdx : 0, vCandinates : [2 ,3 ]  } 

	MSG_ROOM_START_BET,
	// svr : { players : [ { lastOffset : 0 , canTuiZhuang : 1 }, .... ] } ;

	MSG_PLAYER_DO_BET,
	// client : { betTimes : 2 }
	// svr : { ret: 0 } , ret : 0 success , 1 invalid arguments , 2 you are not in room , 3 already beted, 4 state error  ;
	MSG_ROOM_DO_BET,
	// svr : { idx : 2 , betTimes : 23 }
	MSG_ROOM_START_ROBOT_BANKER,
	// svr : null ;
	MSG_PLAYER_ROBOT_BANKER,
	// client : { robotTimes : 2 }
	// svr : { ret : 0 }  ret : 0 success , 1 invalid arguments , 2 you are not in room , 3 already beted, 4 state error  ;
	MSG_ROOM_ROBOT_BANKER,
	// svr : { idx : 2 , robotTimes : 23 }
	MSG_ROOM_DISTRIBUTE_CARD,
	// svr: { info : [ { idx : 0 , cards : [23,23,42,2] }, ..... ] } // info is a array , per item is a player , idx is player idx , cards is current sended hold cards , may be not 5 ;
	MSG_PLAYER_CACULATE_NIU,
	// client : null ;
	// svr : { ret: 0 } , ret : 0 success , 1 you are not in room , 2 already beted ;
	MSG_ROOM_CACULATE_NIU,
	// svr : { idx : 2  }
	MSG_ROOM_NIUNIU_GAME_END, 
	// svr: { bankerIdx : 2 , result : [ { uid : 23 , offset : 23, final : -23 }, .... ] }
	MSG_ROOM_GAME_OVER,
	// svr : { dismissID : 23 , result: [ { uid : 23 , final : -23 }, .... ]  }
	// dismissID is null or 0 , means normal dismiss ;
	// dismissID is 1 , means system dismiss room ;
	// dismissID biger than 1 , means player dismiss room ;

	MSG_REQ_ROOM_ITEM_INFO,
	// client : { roomID : 23 }
	// svr : { state : 2 ,isOpen : 0 , roomID: 23, opts: {} , players: [23,234,23 ..] }

	MSG_NN_PLAYER_UPDATE_TUO_GUAN,
	// client : { isTuoGuan : 0  }
	// svr : { ret : 0 } ;   // 0 success , 1 the same , not change , 2 not in Room , 3 arg error;

	MSG_NN_ROOM_UPDATE_TUO_GUAN,
	// svr : { idx : 0 , isTuoGuan : 0 } 

	MSG_POKER_BJ_BEGIN = 800,
	
	MSG_ROOM_BJ_START_GAME,
	// svr : { vSelfCard : [23,23,23,2,23] }

	MSG_PLAYER_MAKED_GROUP,
	// client : { vCards : [23,23,23,23,23] } 
	// svr : { ret : 0 }
	// ret : 0 success , 1 you are not in this room ,2 cards not fit , 3 already maked card ;

	MSG_ROOM_BJ_MAKE_GROUP_CARD,
	// svr : { idx : 0 }

	MSG_ROOM_BJ_GAME_END,
	//svr : { players : [ { idx : 23 ,offset : 23 ,xiPaiOffset : 2 , xiPaiTypes : [2,23,23] , tongGuanOffset : 23 ,vGuoInfo : [ { type : 23 , offset : 2 , cards : [23,23,21] }, ..... ]  } , .....    ] } 
	// offset 表示玩家最终的输赢,eXiPaiType 定义了喜牌枚举，如果没有就是eXiPai_Max

	MSG_ROOM_BJ_GAME_OVER,
	// svr : { dismissID : 23 , result: [ { uid : 23 , final : -23, win : 0 , lose : -2 , xiPai : -23  }, .... ]  }
	// dismissID is null or 0 , means normal dismiss ;
	// dismissID is 1 , means system dismiss room ;
	// dismissID biger than 1 , means player dismiss room ;

	MSG_POKER_GAME_MSG_MAX = 1000,
	
	// dou di zhu 
	MSG_DDZ_ROOM_WAIT_ROBOT_DZ,
	// svr: { idx : 2 }

	MSG_DDZ_PLAYER_ROBOT_DZ,
	// client: { times : 0 }
	// svr: { ret : 0 }
	// times ; 0 means do not robot , 1 means one times ;
	// ret : 0 success , 1 you are not in room , 2 argument error , 3 you are not cur act player, 4 times must big than previous player, 5 you must rob banker 

 	MSG_DDZ_ROOM_ROBOT_DZ,  // some player roboted dz ;
	// svr : { times : 0 , idx } 

	MSG_DDZ_ROOM_PRODUCED_DZ,
	// svr: { dzIdx : 2 , times : 2 , cards : [23,23,23] }

	MSG_DDZ_ROOM_WAIT_CHU,
	// svr: { idx : 0 }

	MSG_DDZ_PLAYER_SHOW_CARDS,
	// client : null ;
	// svr : { ret : 0  }
	// 0 success , 1 you aret not banker , 2 already show cards , 3 only can shou cards before chu pai , 4 other code ;

	MSG_DDZ_ROOM_SHOW_CARDS,
	// svr : { idx : 2 , cards: [ 23,23,2,23] }

	MSG_DDZ_PLAYER_CHU,
	// client: { cards: [23,23,2] , type : 23 }
	// svr : { ret : 0 } // 0 success , 1 you don't have card , 2 you can not chu card , weight invalid , 3 you are not in room , 4 argument error, 5 you are not current act player; 
	MSG_DDZ_ROOM_CHU,
	// svr: { cards : [ 23,22,23], type : 23, idx : 2 }
	MSG_DDZ_ROOM_RESULT,
	// svr : { bombCnt : 2 , isChunTian : 0 , isMingPai : 1 , bottom : 2 , players : [ { idx : 2 , offset : -2,cards : [23,23,2] }, ..... ]  } 
	
	MSG_DDZ_PLAYER_UPDATE_TUO_GUAN, 
	// client : { isTuoGuan : 0  }
	// svr : { ret : 0 } ;   // 0 success , 1 the same , not change , 2 not in Room , 3 arg error;

	MSG_DDZ_ROOM_UPDATE_TUO_GUAN,
	// svr : { idx : 0 , isTuoGuan : 0 } 
	
	MSG_DDZ_WAIT_PLAYER_CHAO_ZHUANG,
	// svr: null
	
	MSG_DDZ_PLAYER_CHAO_ZHUANG,
	// client : { isChao : 0 }
	// svr : { ret : 0 } // ret : 0 success , 1 not in room , 2 you can not chao 
	
	MSG_DDZ_ROOM_CHAO_ZHUANG,
	// svr: { idx : 0 , isChao : 0 }
	
	MSG_ROOM_DDZ_START_GAME,
	// svr : { vSelfCard : [23,23,23,2,23] }

	MSG_DDZ_WAIT_PLAYER_TI_LA_CHUAI,
	// svr: { waitTiLaChuaiPlayers : [ 2,2]}

	MSG_DDZ_PLAYER_TI_LA_CHUAI,
	// client : { isTiLaChuai : 0 }
	// svr : { ret : 0 } // ret : 0 success , 1 not in room , 2 you can not TiLaChuai 

	MSG_DDZ_ROOM_TI_LA_CHUAI,
	// svr: { idx : 0 , isTiLaChuai : 0 }

	MSG_DDZ_MAX = 1500,
	
		
		
	// mj specail msg ;
	MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD,  // 有人出了一张牌，等待需要这张牌的玩家 操作，可以 碰，杠，胡
	// svr : { invokerIdx : 2,cardNum : 32 , acts : [type0, type 1 , ..] }  ;
	// 这个消息不会广播，只会发给需要这张牌的玩家，cardNum 待需要的牌，type 类型参照 eMJActType

	MSG_PLAYER_WAIT_ACT_AFTER_RECEIVED_CARD,  // 自己获得一张牌，杠或者摸，然后可以进行的操作 杠，胡
	// svr : { acts : [ {act :eMJActType , cardNum : 23 } , {act :eMJActType , cardNum : 56 }, ... ]  }  ;
	// 这个消息不会广播，只会发给当前操作的玩家，acts： 可操作的数组， 因为获得一张牌，以后可以进行的操作很多。cardNum 操作相对应的牌，type 类型参照 eMJActType

	MSG_PLAYER_ACT, // 玩家操作
	// client : { dstRoomID : 2345 ,actType : 0 , card : 23 , eatWith : [22,33] }
	// actType : eMJActType   操作类型，参照枚举值, card 操作的目标牌。eatWith: 当动作类型是吃的时候，这个数组里表示要用哪两张牌吃
	// svr : { ret : 0 }
	// ret : 0 操作成功 , 1 没有轮到你操作 , 2 不能执行指定的操作，条件不满足, 3 参数错误 , 4 状态错误 ;

	MSG_ROOM_ACT,  // 房间里有玩家执行了一个操作
	// svr : { idx : 0 , actType : 234, card : 23, gangCard : 12, eatWith : [22,33], huType : 23, fanShu : 23  }
	// idx :  执行操作的那个玩家的索引。 actType : 执行操作的类型，参照枚举值eMJActType 。 card： 操作涉及到的牌  gangCard: 杠牌后 获得的牌;
	// eatWith : 当吃牌的时候，表示用哪两张牌进行吃
	// huType : 胡牌类型，只有是胡的动作才有这个字段；
	// fanShu :  胡牌的时候的翻数，只有胡牌的动作才有这个字段

	MSG_REQ_ACT_LIST,   //玩家重新上线，断线重连 收到roomInfo 后，发送此消息请求玩家操作列表；
	// client : { dstRoomID : 356 } ,
	// svr : { ret : 0 } ;
	// ret : 0 等待你出牌，只能出牌，1 此刻不是你该操作的时候。
	MSG_SET_NEXT_CARD, // send to mj server 
	 //client : {card : 0,dstRoomID : 123465}

	// new request zhan ji 
	MSG_ROOM_SETTLE_DIAN_PAO, //  实结算的 点炮
	//svr : { paoIdx : 234 , isGangPao : 0 , isRobotGang : 0 , huPlayers : [ { idx : 2 , coin : 2345 }, { idx : 2, coin : 234 }, ... ]  }
	// paoIdx : 引跑者的索引， isGangShangPao ： 是否是杠上炮， isRobotGang ： 是否是被抢杠， huPlayer ： 这一炮 引发的所有胡牌这，是一个数组。 { idx ： 胡牌人的索引， coin 胡牌人赢的金币} 

	MSG_ROOM_SETTLE_MING_GANG, // 实结算 明杠 
	// svr :  { invokerIdx : 234 , gangIdx : 234 , gangWin : 2344 }
	// invokerIdx ： 引杠者的索引， gangIdx ： 杠牌这的索引 ， gangWin： 此次杠牌赢的钱；

	MSG_ROOM_SETTLE_AN_GANG, // 实时结算 暗杠 
	//svr： { gangIdx: 234, losers : [{idx: 23, lose : 234 }, .....] }
	// gangIdx : 杠牌者的索引。 losers 此次杠牌输钱的人，数组。 { idx 输钱人的索引， lose  输了多少钱 }

	MSG_ROOM_SETTLE_BU_GANG, // 实际结算 补杠
	// svr : 参数和解释都跟 暗杠一样。

	MSG_ROOM_SETTLE_ZI_MO, // 实时结算 自摸
	// svr ： { ziMoIdx: 234, losers : [{idx: 23, lose : 234 }, .....] }
	// ziMoIdx : 自摸的人的索引。 losers ： 自摸导致别人数钱了。一个数字。 {idx 输钱人的索引， lose ： 输了多少钱 } 

	MSG_PLAYER_DETAIL_BILL_INFOS, // 游戏结束后收到的个人详细账单，每个人只能收到自己的。
	// svr ： { idx： 23 ， bills：[ { type: 234, offset : -23, huType : 23, beiShu : 234, target : [2, 4] } , .......... ] } 
	// idx : 玩家的索引。
	// bills : 玩家的账单数组，直接可以用于显示。 账单有多条。
	// 账单内解释： type ： 取值参考枚举 eSettleType ， offset ： 这个账单的输赢，负数表示输了， 结合type 得出描述，比如：Type 为点炮，正数就是被点炮，负数就是点炮，
	// 同理当type 是自摸的时候，如果offset 为负数，那么就是被自摸，整数就是自摸。依次类推其他类型。
	// huType : 只有当是自摸的时候有效，表示自摸的胡类型，或者被点炮 这个字段也是有效的。beiShu ：就是胡牌的倍数，有效性随同　ｈｕＴｙｐｅ。 
	// target : 就是自己这个账单 相对的一方， 就是赢了哪些人的钱，或者输给谁了。被谁自摸了，被谁点炮了，点炮了谁。具体到客户端表现，就是最右边那个 上家下家，之类的那一列。

	MSG_ROOM_PLAYER_CARD_INFO,
	// svr : { idx : 2, queType: 2, anPai : [2,3,4,34], mingPai : [ 23,67,32] , huPai : [1,34], chuPai: [2,34,4] },{ anPai : [2,3,4,34], mingPai : [ 23,67,32], anGangPai : [23,24] , huPai : [1,34] }
	//  anPai 就是牌，没有展示出来的，mingPai 就是已经展示出来的牌（碰，杠），huPai ： 已经胡了的牌。 queType : 1,万 2, 筒 3, 条
	// anGangPai : 就是安杠的牌，单张，不是4张。比如 暗杠8万，那么就是一个8万，也不是4个8万。

	// NanJing ma jiang :
	// svr: { idx : 2 , newMoCard : 2, anPai : [2,3,4,34] , chuPai: [2,34,4] , huaPai: [23,23,23] , anGangPai : [23,24],buGang : [23,45] ,pengGangInfo : [ { targetIdx : 23 , actType : 23 , card : 23 } , .... ]  }
	// idx ： 玩家索引,  anPai 就是牌，没有展示出来的, chuPai ： 就是已经出了的牌。buGang : 补杠的牌
	// newMoCard : 最新摸的牌，可能是杠 或者 摸牌
	// pengGangInfo: 杠牌和碰牌的数组。{ targetIdx ： 23， actType ： 23 ， card ： 234 } 分别是： 触发动作的索引，actType ， 就是碰了 还是杠了，card 就是哪张牌；

	// SuZhou ma jiang
	// svr: { idx : 2 , newMoCard : 2, anPai : [2,3,4,34] , chuPai: [2,34,4] , huaPai: [23,23,23] , anGangPai : [23,24],buGang : [23,45], pengCard : [23,45] }
	// idx ： 玩家索引,  anPai 就是牌，没有展示出来的, chuPai ： 就是已经出了的牌。buGang : 补杠的牌, pengCard: 碰的牌
	// newMoCard : 最新摸的牌，可能是杠 或者 摸牌

	MSG_MJ_ROOM_INFO,  // 房间的基本信息
	// svr : { roomID ： 23 , configID : 23 , waitTimer : 23, bankerIdx : 0 , curActIdex : 2 , leftCardCnt : 23 , roomState :  23 , players : [ {idx : 0 , uid : 233, coin : 2345 , state : 34, isOnline : 0  }, {idx : 0 , uid : 233, coin : 2345, state : 34, isOnline : 0 },{idx : 0 , uid : 233, coin : 2345 , state : 34,isOnline : 0 } , ... ] }
	// roomState  , 房间状态
	// isOnline : 玩家是在线 ， 1 是在线， 0 是不在线
	// leftCardCnt : 剩余牌的数量，重新进入已经在玩的房间，或者断线重连，就会收到这个消息，
	// bankerIdx : 庄家的索引
	// curActIdx :  当前正在等待操作的玩家

	

	MSG_VIP_ROOM_CLOSED,
	// { uid : 2345 , roomID : 2345 , eType : eroomType } 

	MSG_ROOM_PLAYER_ENTER, // 有其他玩家进入房间
	// svr : {idx : 0 , uid : 233, coin : 2345,state : 34 }

	MSG_ROOM_PLAYER_LEAVE, // 有玩家离开房间;
	// svr : { idx : 2 ,isExit : 0 }
	// isExit : 是否是离开，还是真的退出。 1 是真的退出 。一定要判断 isExit 这个值是存在，并且是值为 1 。

	MSG_ROOM_NJ_PLAYER_HU, // 南京麻将玩家胡牌 
	// svr : { isZiMo : 0 , detail : {}, realTimeCal : [ { actType : 23, detial : [ {idx : 2, offset : -23 } ]  } , ... ] }
	//  当是自摸的时候，isZiMo : 1 , detail = { huIdx : 234 , isKuaiZhaoHu : 0, baoPaiIdx : 2 , winCoin : 234,huardSoftHua : 23, gangKaiCoin : 0 ,vhuTypes : [ eFanxing , ], LoseIdxs : [ {idx : 1 , loseCoin : 234 }, .... ]   }
	// 当不是自摸的时候，isZiMo : 0 , detail = { dianPaoIdx : 23 , isRobotGang : 0 , nLose : 23, nWaiBaoLose : 23 huPlayers : [{ idx : 234 , win : 234 , baoPaiIdx : 2  , isKuaiZhaoHu : 0, huardSoftHua : 23, vhuTypes : [ eFanxing , ] } , .... ] } 
	//	isKuaiZhaoHu : 是否是快照胡牌
	// huPlayers : json 数组包含子类型，表示胡牌玩家的数组，一炮多响，有多个胡牌玩家 
	// 胡牌子类型: idx :胡牌玩家的idx ， huardSoftHua : 花数量，offset ：胡牌玩家赢的钱，gangFlag ，胡牌玩家是否是杠开， vhuTypes 是一个数组，表示胡牌时候的 各种翻型叠加, baoPaiIdx : 包牌者的索引，只有包牌情况，才有这个key值，引用钱要判断
	// invokerIdx : 点炮者的UID,  InvokerGangFlag : 放炮者 是不是被抢杠。 当自摸的时候，这个idx 和 胡牌的玩家是一样的。
	// realTimeCal :实时结算的信息 是一个数组 包含每一次的子类型详情；
	// 实时结算子类型是：actType 是什么类型时间导致的结算，参考eMJActType， detial： 也是一个数组 表示，这次结算每个玩家的输赢，idx 玩家的索引，offset，表示加钱 还是减钱，正负表示。

	MSG_ROOM_NJ_GAME_OVER, // 南京麻将结束
	// svr: { isLiuJu : 0 , isNextBiXiaHu : 0 , detail : [ {idx : 0 , offset : 23, waiBaoOffset : 234 }, ...  ], realTimeCal : [ { actType : 23, detial : [ {idx : 2, offset : -23 } ]  } , ... ] } 
	// svr : isLiuJu : 是否是流局
	// detail : 数组就是每个玩家的本局的最终输赢 ；
	// realTimeCal : 实时结算信息，只有流局的情况才存在这个字段；
	// isNextBiXiaHu : 下一局是否要比下胡

	MSG_ROOM_NJ_REAL_TIME_SETTLE, // 南京麻将实时结算
	// svr : {  actType : 0 , winers : [ {idx : 2, offset : 23}, .... ] , loserIdxs : [ {idx : 2 , offset : 23} , ... ]  } } 
	// actType 此次结算的原因是什么，参考eMJActType ;
	// winers : 所有赢钱人的数组 ， { 赢钱人的索引， 赢了多少钱 }
	// loserIdxs : 所有输钱人的数组， { 输钱的索引， 输了多少钱 } 

	MSG_REQ_MJ_ROOM_BILL_INFO,  // 请求vip 房间的账单, 此消息发往麻将游戏服务器；
	// client : { sieral : 2345 }
	// svr : { ret : 0 , sieral : 234, billTime : 23453, roomID : 235, roomType : eRoomType , creatorUID : 345 , circle： 8 ，initCoin : 2345 , detail : [  { uid : 2345 , curCoin : 234, ziMoCnt : 2 , huCnt : 23,dianPaoCnt :2, mingGangCnt : 23,AnGangCnt : 23  }, ....]  } 
	// ret : 0 成功，1 账单id不存在，billID, 账单ID， billTime ： 账单产生的时间, roomID : 房间ID ， roomType 房间类型eRoomType， creatorUID 创建者的ID，circle 房间的圈数，initCoin ： 初始金币，detail : 每个人的输赢详情 json数组
	// uid : 玩家的uid，curCoin 结束时剩余钱；

	// su zhou ma jiang
MSG_ROOM_SZ_PLAYER_HU, // 苏州麻将玩家胡牌 
// svr : { isZiMo : 0 ,isFanBei : 0 , detail : {} }
//  当是自摸的时候，isZiMo : 1 , detail = { huIdx : 234 , winCoin : 234,huHuaCnt : 23,holdHuaCnt : 0, isGangKai :0 , invokerGangIdx : 0, vhuTypes : [ eFanxing , ] }
// 当不是自摸的时候，isZiMo : 0 , detail = { dianPaoIdx : 23 , isRobotGang : 0 , nLose : 23, huPlayers : [{ idx : 234 , win : 234 , huHuaCnt : 23,holdHuaCnt : 0, vhuTypes : [ eFanxing , ] } , .... ] } 
// huPlayers : json 数组包含子类型，表示胡牌玩家的数组，一炮多响，有多个胡牌玩家 
// 胡牌子类型: idx :胡牌玩家的idx ， huaCnt : 花数量，offset ：胡牌玩家赢的钱，isGangKai ，胡牌玩家是否是杠开， vhuTypes 是一个数组，表示胡牌时候的 各种翻型叠加,
// invokerGangIdx : 引杠者的索引，当明杠，直杠才有这个key值,暗杠的时候这个就是胡牌者自己

MSG_ROOM_SZ_GAME_OVER, // 苏州麻将结束
// svr: { isLiuJu : 0 , isNextFanBei : 0 , detail : [ {idx : 0 , offset : 23 }, ...  ] } 
// svr : isLiuJu : 是否是流局
 // detail : 数组就是每个玩家的本局的最终输赢 ；
 // isNextFanBei : 下一局是否要翻倍

	MSG_ROOM_UPDATE_PLAYER_NET_STATE, // 更新房间内玩家的在线状态
	// svr : { idx : 0 , isOnLine : 0 } // isOnline 0 不在线，1 在线 。  

	MSG_REQ_ZHAN_JI, // send to mj server 
	// client : { userUID : 234 , curSerial : 2345 }
	// svr : { nRet : 0 ,sieral : 2345 ,cirleCnt : 2, roomID : 235 , createrUID : 234, roomOpts : {} , rounds : [ { replayID : 234, time : 234 , result : [ { uid :234， offset ： 234  }, ... ]  }, .... ]    } 
	//	nRet: 0 成功， 1 找不到战绩, 2 uid 不没参与制定战绩房间
	// userUID : 请求者的Uid ， curSerial ： 客户端当前的 的序列号，返回的 战绩从这个序列号开始
	// sieral : 当前返回战绩的房间序列号，roomOpts ： 不同的游戏参数不一样，
	// cirleCnt ： 圈数 或者 局数
	//  rounds ： 每一局的战绩详情数组，数组内 replayID 回放ID， time 结束时间，result： 每个玩家的输赢记录数组，{ 玩家id ， 玩家输赢} 

	// 当南京麻将的时候，opts ： { isBiXiaHu : 0 , isHuaZa : 0 , isKuaiChong : 0 , kuaiChongPool : 234 , isJinYuanZi : 0 , yuanZi : 200 }
	// 每个局 每个玩家的输赢 多一个 特殊的key值较 waiBaoOffset : 表示外包输赢； 

	MSG_CHANGE_BAOPAI_CARD,
	// svr: {idx : 0,card : 0}
	//card : 0是取消，其它的是包牌的card

	MSG_ROOM_GOLDEN_BEGIN = 1700, //三张命令号开始标识

	MSG_ROOM_GOLDEN_GAME_END, //三张游戏结束消息
	// svr: { bankerIdx : 2 , result : [ { uid : 23 , offset : 23, final : -23 }, .... ] }

	MSG_ROOM_GOLDEN_GAME_WAIT_ACT, //三张发送玩家操作列表消息
	// sur: { acts : { act ： 1 , info : 1 } , { act : 2 , info : 1 } ... }

	MSG_ROOM_GOLDEN_GAME_PASS, //三张玩家弃牌
	// sur : { ret : 0 }  0, 成功; 1, 玩家为空.

	MSG_ROOM_GOLDEN_GAME_CALL, //三张玩家跟注
	// sur : { ret : 0 , idx : 1 , coin : 10 , (mutiple : 1)//只有在加注时会发 }

	MSG_ROOM_GOLDEN_GAME_CALL2END, //三张玩家更改一跟到底
	// sur : { ret : 0, call2end : 1 }

	MSG_ROOM_GOLDEN_GAME_LOOK_CARDS, //三张看牌
	// sur : { ret : 0, (idx : 1)//群发用于其他玩家知晓, (cards : { 23 , 24, 25 })//用于发送给看牌的玩家知晓 } 当失败时只给要求看牌玩家发失败信息(只有ret)

	MSG_ROOM_GOLDEN_GAME_PK, //三张比牌
	// sur : { ret : 0, idx : 1, withIdx : 2, result : 1 }
	// result : 1, 胜利; 0, 失败

	MSG_ROOM_GOLDEN_GAME_END_PK, //三张最终PK
	// sur : { participate : { 1 , 2, 3 } , lose : { 2 , 3 } }
	// participate : 参与者
	// lose : 输的人

	MSG_ROOM_GOLDEN_END = 1900, //三张命令号结束标识



	MSG_ROOM_SICHUAN_MAJIANG_BEGIN = 2000, //四川麻将命令号开始标记

	MSG_ROOM_SCMJ_GAME_END, //四川麻将游戏结束

	MSG_ROOM_SCMJ_PLAYER_HU, //四川麻将胡

	MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, //四川麻将换三张

	MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS, //四川麻将定缺

	MSG_ROOM_SCMJ_GAME_START, //四川麻将开始游戏消息

	MSG_ROOM_SICHUAN_MAJIANG_END = 2100, //四川麻将命令号结束标识


	/*13水消息列表
		(2200 - 2300)
		ret: 0成功, >1 失败
	*/
	MSG_ROOM_THIRTEEN_BEGIN = 2200, //13水命令号开始标记

	MSG_ROOM_THIRTEEN_GAME_END, //13水游戏结束
	// svr: { result : [ { idx : 23 , offset : 23, cards : [[1,2,3], [4,5,6,7,8], [9,10,11,12,13]], cardsType : [1,1,1], cardsWeight : [12,12,12], swat : 0 , shoot : [0 , 1 , 2] }, .... ] }
	// swat 红波浪：值为玩家IDX
	// shoot 打枪 : 数组被打枪玩家的IDX

	MSG_ROOM_THIRTEEN_GAME_WAIT_ACT, //动作列表（开始信号）
	// null

	MSG_ROOM_THIRTEEN_GAME_PUT_CARDS, //十三张玩家摆牌
	// client: { cards : [1, 2, 3, ... , 13] }
	// svr : { ret : 1 } 正确时无返回，错误时返回为大于0的值
	// ret : 1, 找不到该玩家  2, 牌型信息错误  3, 摆牌错误，无法摆牌  4, 已摆牌

	MSG_ROOM_THIRTEEN_GAME_PUT_CARDS_UPDATE, //十三张玩家摆牌更新
	// svr : { idx : 1 , state : 0/1 , sys : 1}
	// sys : system auto put cards, if not do not send this key

	MSG_ROOM_THIRTEEN_GAME_SHOW_CARDS, //十三张玩家明牌
	// client: null
	// svr : { ret : 1 }
	// 1, 玩家不存在  2, 状态错误操作失败  3, 当前房间不能明牌  4, 金币不足  7, 请求超时

	MSG_ROOM_THIRTEEN_GAME_DELAY_PUT, //十三张增加摆牌时间
	// client: 暂时定义null, 每次增加固定的时间
	// svr : { ret : 1 }
	// 1, 玩家不存在 

	MSG_ROOM_THIRTEEN_UPDATE_CARDS_PUT_TIME, //十三张摆牌时间更新
	// svr : {idx : 1, time : int}

	MSG_ROOM_THIRTEEN_START_ROT_BANKER, //十三水开始抢庄信号
	// svr : {}

	MSG_ROOM_THIRTEEN_ROT_BANKER, //十三张抢庄
	// client: {state : 1}
	// state : 1, rot banker; 0, do not rot banker
	// svr : { ret : 0 }
	// 1, 玩家不存在  2, 状态错误操作失败  3, 当前房间不能抢庄  4, 金币不足  5, 积分不足  7, 请求超时 

	MSG_ROOM_THIRTEEN_PRODUCED_BANKER, //十三水庄信息
	// svr : {bankerIdx : 1, rotBanker : 1}
	// rotBanker : 1, banker is rotted; 0, normal banker

	//MSG_ROOM_THIRTEEN_SHOW_CARDS, //十三水明牌命令
	// client : {}
	// svr : {ret : 0}

	MSG_ROOM_THIRTEEN_SHOW_CARDS_UPDATE, //十三水玩家明牌更新
	// svr : {idx : 1, cards : [1,2, ... 13], waitTime: int}

	MSG_ROOM_THIRTEEN_GOLDEN_UPDATE, //十三张游戏内剩余金币更新
	// svr : { idx : 1 , chips : 123 }

	MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, //申请带入金币
	// client : {amount : 100, clubID : 123}
	// svr : {ret : 0}
	// 1, 玩家不存在  2 3, 带入金额错误  4, 金币不足  5 6, 俱乐部选择错误  7, 请求超时

	MSG_ROOM_THIRTEEN_DRAG_IN,//带入金币反馈信息
	// svr : {idx : 1 , chips : 123}

	MSG_ROOM_THIRTEEN_NEED_DRAGIN, //需要带入金币才能继续
	// svr : {idx : 1, clubIDs : [123,123,123], min : 100, max : 200}

	MSG_ROOM_THIRTEEN_REAL_TIME_RECORD, //十三水实时战绩
	// svr : {idx : 0, detail : [{uid : 123, chip : 123, drag : 123}, ...]}
	// 10 tips per page

	MSG_ROOM_THIRTEEN_STAND_PLAYERS, //十三水围观玩家信息
	// client : {}
	// svr : {idx : 0, players : [123, 123, 123...]}
	// 20/per page

	MSG_ROOM_THIRTEEN_BOARD_GAME_RECORD, //十三水上局回顾
	// client : {idx : -1}
	// -1 : last game (stat form 0)
	// svr : {ret : 0, idx : 123, detail : [{uid : 123, offset : 123, cards : [12, 12, ...], types : [1, 2, 3]}, ...]}

	MSG_ROOM_REQUEST_THIRTEEN_ROOM_INFO, //十三水请求房间基本信息
	// client : as request room info
	// svr : {ret : 0, roomID : 123, leftTime : 123, opts : {json::opts}}

	MSG_ROOM_THIRTEEN_PLAYER_AUTO_STANDUP, //十三水玩家自动站起切换
	// clinet : {state : 1}
	// state : 1 auto stand up,  0 cancle auto stand up
	// svr : {ret : 0, state : 1}
	// 1, 玩家不存在  2, 操作错误

	MSG_ROOM_THIRTEEN_PLAYER_AUTO_LEAVE, //十三水玩家自动离开切换
	// clinet : {state : 1}
	// state : 1 auto leave,  0 cancle auto leave
	// svr : {ret : 0, state : 1}
	// 1, 玩家不存在  2, 操作错误

	MSG_ROOM_THIRTEEN_CLIENT_OVER, //十三水游戏客户端结束
	// client : {}

	MSG_ROOM_THIRTEEN_DECLINE_DRAG_IN, //带入金币拒绝反馈信息
	// svr : {}

	MSG_ROOM_THIRTEEN_REPUT_CARDS, //十三水游戏重新摆牌
	// client : {}
	// svr : {ret : 0}
	// 1, 玩家不存在  2, 操作错误  3, 未摆牌

	MSG_ROOM_THIRTEEN_RBPOOL_UPDATE, //十三张抢庄池发生变化
	// svr : {pool : 123}

	MSG_ROOM_THIRTEEN_CANCEL_DRAGIN, //十三水取消带入
	// client : {}

	MSG_ROOM_THIRTEEN_END = 2300, //13水命令号结束标记



	/*Club消息列表
		(2400 - 2500)
		ret: 0成功, >1 失败
	*/
	MSG_CLUB_MESSAGE_BEGIN = 2400,

	MSG_CLUB_PLAYER_CLUB_INFO, //玩家俱乐部信息
	//client : {nTargetID : playerUID}
	//svr : {joined : [111, 222, 333] , created : [111, 222, 333]}

	MSG_CLUB_CREATE_CLUB, //创建俱乐部
	//client : targetID: playerUID, {name : "str", region : "str", description : "null", icon : "str"}
	//svr : {ret : 0, clubID : 123}
	// 1, 俱乐部名为空  2, 地区为空  3, 俱乐部已存在

	MSG_CLUB_DISMISS_CLUB, //解散俱乐部
	//client : targetID: clubID, {uid : 123}
	//svr : {ret : 0, clubID : 123}
	// 1, 俱乐部错误  2, 权限不足  3, 操作失败

	MSG_CLUB_APPLY_JOIN, //玩家申请加入俱乐部
	//client : targetID: clubID, {uid : 123}
	//svr : {ret : 0}
	// 1, 俱乐部错误  2, 已加入  3, 已申请

	MSG_CLUB_FIRE_PLAYER, //踢出玩家
	//client : targetID: clubID, {uid : 123, fireUID : 123}
	//svr : {ret : 0}
	// 1 2, 玩家信息错误  3, 权限不足  4, 操作无效

	MSG_CLUB_QUIT_CLUB, //玩家请求退出俱乐部
	//client : targetID: clubID, {uid : 123}
	//svr : {ret : 0}
	// 1, 玩家信息错误  2, 俱乐部群主无法退出  3, 操作无效
	
	MSG_CLUB_APPLY_CLUB_INFO, //玩家申请俱乐部信息
	//client : targetID: clubID, {}
	//svr : {ret : 0, clubID : 123, name : "str", creator : 123, nom : 12, lom : 12, region : "str", description : "str", icon : "url", nor : 123}
	//nom : number of member	lom : limit of member amount	nor : number of room

	MSG_CLUB_APPLY_CLUB_DETAIL, //玩家申请俱乐部详情
	//client : targetID: clubID, {}
	//svr : {ret : 0, creator : 123, members : [{id : 123, level : 1}, {id : 321, level : 1}, ...], createType : 1, searchLimit : 1, foundation : 123}
	//createType : who can create room (0: everyone, 1: administrator or creator, 2: creator)
	//searchLimit : who can search dao this club(0: everyone, 1: nobody)

	MSG_CLUB_APPLY_ROOM_INFO, //玩家请求俱乐部牌局信息
	//client : targetID: clubID, {}
	//svr : {ret : 0, clubID : 123, rooms : [{id : 123, port : 13}, {id : 321, port : 12}, ...]}
	// 1, 连接超时，牌局信息可能不全

	MSG_CLUB_INFO_UPDATE_ICON, //修改头像
	//client : targetID: clubID, {uid : 123, icon : "url"}
	//svr : {ret : 0, clubID : 123}
	// 1, 玩家信息错误  2, 权限不足

	MSG_CLUB_INFO_UPDATE_NAME, //修改名称
	//client : targetID: clubID, {uid : 123, name : "str"}
	//svr : {ret : 0, clubID : 123}
	// 1, 玩家信息错误  2, 权限不足  3, 名字为空

	MSG_CLUB_INFO_UPDATE_CREATE_TYPE, //修改建房权限
	//client : targetID: clubID, {uid : 123, state : 1}
	//svr : {ret : 0, clubID : 123, state : 1}
	// 1, 玩家信息错误  2, 权限不足  3 4, 信息有误

	MSG_CLUB_INFO_UPDATE_SEARCH_LIMIT, //修改搜索限制
	//client : targetID: clubID, {uid : 123, state : 1}
	//svr : {ret : 0, clubID : 123, state : 1}
	// 1, 玩家信息错误  2, 权限不足  3 4, 信息有误

	MSG_CLUB_INFO_UPDATE_DESCRIPTION, //修改描述
	//client : targetID: clubID, {uid : 123, description : "str"}
	//svr : {ret : 0, clubID : 123}
	// 1, 玩家信息错误  2, 权限不足  3 4, 信息有误

	MSG_CLUB_INFO_UPDATE_LEVEL, //修改玩家等级权限
	//client : targetID: clubID, {uid : 123, memberUID : 321, level : 1}
	//svr : {ret : 0}
	// 1 3, 玩家信息错误  2, 权限不足  4, 玩家未加入  5, 信息错误  6, 操作无效

	MSG_CLUB_EVENT_GRANT_FOUNDATION, //发放基金
	//client : targetID: clubID, {uid : 123, memberUID : 321, amount : 1000}
	//svr : {ret : 0}
	// 1, 玩家信息错误  2, 权限不足  3, 信息有误  4, 玩家未加入  5, 金额错误  6, 基金不足

	MSG_CLUB_EVENT_GRANT_RECORDER, //俱乐部发放基金记录
	//client : targetID: clubID, {}
	//svr : {ret : 0, clubID : 123, events : [{eventID : 123, time: 123456, disposer:123, detail : {json}}, ... ]}

	MSG_CLUB_EVENT_JOIN, //申请加入消息列表
	//client : targetID: clubID, {}
	//svr : {ret : 0, events : [{eventID: 123, time: 123456, state: 0, disposer: 123, detail : {json}}, ...]}
	//state : 0, wait access	1, apply accede		2, apply refused
	//disposer : treat uid, if 0 no body or treat by system
	//detail : json received from client

	MSG_CLUB_EVENT_ACTIVE_UPDATE, //服务器主动推送事件列表更新
	//svr : {clubID : 123, type : 0}
	//type : eClubEventType

	MSG_CLUB_EVENT_ENTRY, //申请带入消息列表
	//client : targetID: clubID, {}
	//svr : {ret : 0, events : [{eventID: 123, time: 123456, detail : {json}}, ...]}

	MSG_CLUB_EVENT_ENTRY_UPDATE, //服务器接受客户端申请事件列表更新
	//client : targetID: clubID, {uid : 123}
	//svr : {ret : 0, clubID : 123, detail : [{type : 0, amount : 0}, ...]}

	MSG_CLUB_EVENT_ENTRY_RECORDER, //申请带入消息记录列表
	//client : targetID: clubID, {}
	//svr : {ret : 0, events : [{eventID: 123, time: 123456, state: 0, disposer: 123, detail : {json}}, ...]}
	//state : 0, wait access	1, apply accede		2, apply refused
	//disposer : treat uid, if 0 no body or treat by system
	//detail : json received from client

	MSG_CLUB_EVENT_ENTRY_RECORDER_UPDATE, //申请带入记录列表更新
	//wait

	MSG_CLUB_EVENT_APPLY_TREAT, //申请处理俱乐部事件
	//client : targetID: clubID, {uid : 123, eventID : 321, state : 0}
	//svr : {ret : 0}
	//state : 1, accede		2, refused
	// 1, 玩家信息错误  2 3 5 10, 信息错误  6, 事件已处理  7, 权限不足  8, 操作错误  9, 事件类型错误  11, 联盟积分不足  12, 请求超时  13, 该玩家金币不足

	MSG_LEAGUE_CLUB_LEAGUE_INFO, //俱乐部联盟信息
	//client : targetID: clubID, {}
	//svr : {joined : [111, 222, 333] , created : [111, 222, 333]}

	MSG_LEAGUE_CREATE_LEAGUE, //创建联盟部
	//client : targetID: clubID, {uid : 123, name : "str", icon : "url:null"}
	//svr : {ret : 0}
	// 1, 名字为空  3, 联盟已存在

	MSG_LEAGUE_APPLY_LEAGUE_INFO, //玩家申请联盟信息
	//client : targetID: leagueID, {}
	//svr : {ret : 0, leagueID : 123, name : "str", creator : 123}

	MSG_LEAGUE_APPLY_LEAGUE_DETAIL, //玩家申请联盟详情
	//client : targetID: leagueID, {}
	//svr : {ret : 0, leagueID : 123, name : "str", creator : 123, members : [{id : 123, level : 1}, {id : 321, level : 1}, ...], joinLimit : 1, joinEvents : [{eventID : 123, time : 123, detail : {json}}, ...]}
	//joinLimit : 0, all club can join		1, no one can search dao

	MSG_LEAGUE_JOIN_LEAGUE, //加入联盟
	//client : targetID: leagueID, {uid : 123, clubID : 123}
	//svr : {ret : 0}
	// 1, 俱乐部信息错误  2, 玩家信息错误  3, 已申请  4, 权限不足  6, 联盟拒绝请求  7, 请求超时

	MSG_LEAGUE_UPDATE_JOIN_LIMIT, //修改联盟搜索限制
	//client : targetID: leagueID, {uid : 123, clubID : 123, state : 1}
	//svr : {ret : 0, clubID : 123, state : 1}
	// 1, 俱乐部信息错误  2, 玩家信息错误  3 4, 信息错误  5, 权限不足

	MSG_LEAGUE_FIRE_CLUB, //踢出俱乐部
	//client : targetID: leagueID, {uid : 123, clubID : 123, fireCID : 123}
	//svr : {ret : 0}
	// 1, 俱乐部信息错误  2, 玩家信息错误  3 4, 信息错误  5 6, 权限不足  7, 请求超时

	MSG_LEAGUE_DISMISS_LEAGUE, //解散联盟
	//client : targetID: leagueID, {uid : 123, clubID : 123}
	//svr : {ret : 0}
	// 1, 俱乐部信息错误  2, 玩家信息错误  3, 信息错误  5, 权限不足  7, 请求超时

	MSG_LEAGUE_QUIT_LEAGUE, //退出联盟
	//client : targetID: leagueID, {uid : 123, clubID : 123}
	//svr : {ret : 0}
	// 1, 俱乐部信息错误  2, 玩家信息错误  3, 信息错误  5, 操作失败  7, 请求超时

	MSG_LEAGUE_EVENT_JOIN, //申请加入联盟列表
	//client : targetID: leagueID, {}
	//svr : {ret : 0, events : [{eventID: 123, time: 123456, state: 0, disposer: 123, detail : {json}}, ...]}
	//state : 0, wait access	1, apply accede		2, apply refused
	//disposer : treat uid, if 0 no body or treat by system
	//detail : json received from client

	MSG_LEAGUE_EVENT_APPLY_TREAT, //申请处理联盟事件
	//client : targetID: leagueID, {uid : 123, eventID : 321, clubID : 123, state : 0}
	//svr : {ret : 0}
	//state : 1, accede		2, refused
	// 1, 玩家信息错误  2 4 5 8, 信息错误  3, 俱乐部信息错误  6 10, 权限不足  7, 请求超时  9, 事件已处理  11, 操作失败  12, 事件无法处理

	MSG_LEAGUE_EVENT_ACTIVE_UPDATE, //服务器主动推送联盟事件列表更新
	//svr : {leagueID : 123, clubID : 123, type : 0}

	MSG_LEAGUE_EVENT_ACTIVE_LIST_UPDATE, //服务器接受客户端申请联盟事件列表更新
	//client : targetID: leagueID, {uid : 123, clubID : 123}
	//svr : {ret : 0, clubID : 123, leagueID : 123, detail : [{type : 0, amount : 0}, ...]}

	MSG_CLUB_MESSAGE_END = 2500,

	MSG_PLAYER_RESET_PASSWORD, //玩家重置密码
	//client : {number : 11111111, password : "123456"}
	//DATASERVER消息 targetID 发玩家ID
	//svr : {ret : 0} 0成功，大于0失败



































































	///------new define end---
	MSG_SERVER_AND_CLIENT_COMMON_BEGIN,  // server and client common msg , beyond specail game 
	MSG_PLAYER_ENTER_GAME,    // after check , enter game 
	
	//MSG_PLAYER_CONTINUE_LOGIN,  // contiune login prize ;
	
	MSG_CREATE_ROLE,
	// player base Data 
	
	MSG_SHOW_CONTINUE_LOGIN_DLG,
	MSG_GET_CONTINUE_LOGIN_REWARD,

	MSG_PLAYER_UPDATE_VIP_LEVEL,


	// slot machine 
	MSG_PLAYER_SLOT_MACHINE, // lao hu ji ;

	// item 
	MSG_REQUEST_ITEM_LIST ,
	MSG_SAVE_ITEM_LIST,
	MSG_PLAYER_PAWN_ASSERT, //  dian dang zi chan
	MSG_PLAYER_USE_GIFT,
	// rank
	MSG_REQUEST_RANK,
	MSG_REQUEST_RANK_PEER_DETAIL,
	// inform 
	MSG_INFORM_NEW_NOTICES ,
	MSG_PLAYER_REQUEST_NOTICE,
	MSG_GLOBAL_BROCAST,
	MSG_PLAYER_SAY_BROCAST,
	// shop 
	MSG_SAVE_SHOP_BUY_RECORD,
	MSG_GET_SHOP_BUY_RECORD,
	MSG_PLAYER_REQUEST_SHOP_LIST,
	
	MSG_PLAYER_RECIEVED_SHOP_ITEM_GIFT,
	// mission 
	MSG_GAME_SERVER_SAVE_MISSION_DATA,
	MSG_GAME_SERVER_GET_MISSION_DATA,
	MSG_PLAYER_REQUEST_MISSION_LIST,
	MSG_PLAYER_NEW_MISSION_FINISHED,
	MSG_PLAYER_REQUEST_MISSION_REWORD,

	// online box 
	MSG_PLAYER_REQUEST_ONLINE_BOX_REWARD,
	MSG_PLAYER_REQUEST_ONLINE_BOX_STATE,

	// room common msg ;
	MSG_ROOM_MSG_BEGIN,
	MSG_ROOM_RET,
	MSG_ROOM_SPEAK,
	MSG_ROOM_OTHER_SPEAK,  
	MSG_ROOM_REQUEST_PEER_DETAIL,
	MSG_ROOM_KICK_PEER,
	MSG_ROOM_OTHER_KICK_PEER,
	MSG_ROOM_EXE_BE_KICKED,
	MSG_ROOM_PROCESSE_KIKED_RESULT,

	MSG_ROOM_ENTER,
	//MSG_ROOM_PLAYER_ENTER,  // MSG_ROOM_PLAYER_x means other player actions 
	MSG_PLAYER_FOLLOW_TO_ROOM, // zhui zong pai ju 

	MSG_ROOM_LEAVE,
	//MSG_ROOM_PLAYER_LEAVE,
	// private room 
	MSG_PLAYER_CREATE_PRIVATE_ROOM,  // create private Room ;

	// message for robot 
	MSG_ROBOT_ORDER_TO_ENTER_ROOM = 25000,
	MSG_ROBOT_APPLY_TO_LEAVE,
	MSG_ROBOT_CHECK_BIGGIEST,
	MSG_ROBOT_INFORM_IDLE,
	
	// all room msg above ,

	// golden room 
	MSG_GOLDEN_ROOM_ENTER,
	MSG_GOLDEN_ROOM_LEAVE,
	MSG_GOLDEN_ROOM_INFO,
	MSG_GOLDEN_ROOM_STATE,

	MSG_GOLDEN_ROOM_PLAYER_SHOW_CARD,
	MSG_GOLDEN_ROOM_SHOW_CARD,

	MSG_GOLDEN_ROOM_PLAYER_CHANGE_CARD,
	MSG_GOLDEN_ROOM_CHANGE_CARD,

	MSG_GOLDEN_ROOM_PLAYER_PK_TIMES,
	MSG_GOLDEN_ROOM_PK_TIMES,

	MSG_GOLDEN_ROOM_PLAYER_READY,
	MSG_GOLDEN_ROOM_READY,

	
	MSG_GOLDEN_ROOM_INFORM_ACT,
	
	//MSG_GOLDEN_ROOM_PLAYER_LOOK,
	//MSG_GOLDEN_ROOM_LOOK,
	
	MSG_GOLDEN_ROOM_PLAYER_GIVEUP,
	MSG_GOLDEN_ROOM_GIVEUP,

	MSG_GOLDEN_ROOM_PLAYER_FOLLOW,
	MSG_GOLDEN_ROOM_FOLLOW,

	MSG_GOLDEN_ROOM_PLAYER_ADD,
	MSG_GOLDEN_ROOM_ADD,

	MSG_GOLDEN_ROOM_PLAYER_PK,

	MSG_GOLDEN_ROOM_RESULT,
};
