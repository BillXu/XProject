#pragma once
enum DDZ_Type
{
	DDZ_Single,
	DDZ_Pair,
	DDZ_3Pices,
	DDZ_3Follow1,
	DDZ_SingleSequence,
	DDZ_PairSequence,
	DDZ_3PicesSeqence,
	DDZ_AircraftWithWings,
	DDZ_4Follow2,
	DDZ_Common,
	DDZ_Bomb,
	DDZ_Rokect,
	DDZ_Max,
};

enum eFALGroupCardType
{
	eFALCardType_None,
	eFALCardType_Single,//单张
	eFALCardType_Double,//对子
	eFALCardType_ThreeCards,//三张
	eFALCardType_ThreeBySingle,//三带一
	eFALCardType_ThreeByDouble,//三带对
	eFALCardType_FourBySingle,//四带两单
	eFALCardType_FourByDouble,//四带两对
	eFALCardType_Straight,//顺子
	eFALCardType_DoubleStraight,//对子顺
	eFALCardType_ThreeStraight,//三张顺
	eFALCardType_ThreeStraightBySingle,//飞机带单张
	eFALCardType_ThreeStraightByDouble,//飞机带对子
	eFALCardType_NotBomb = eFALCardType_ThreeStraightByDouble,//不是炸弹
	eFALCardType_FourBomb,//4同炸弹
	eFALCardType_JokerBomb,//双王火箭
	eFALCardType_Max,
};

enum DDZ_FRAME
{
	DDZ_Frame_WaitChaoZhuang, // [ { idx : 2, uid : 23 }, .... ]
	DDZ_Frame_DoChaoZhuang, // { idx : 0 }
	DDZ_Frame_StartGame, // [ { idx : 0 , uid : 23, cards : [23,23,23] }, .... ]
	DDZ_Frame_WaitRobBanker,// { idx : 2 } 
	DDZ_Frame_DoRobBanker, // { idx : 2 , times : 1 }
	DDZ_Frame_DoProducedBanker, // { dzIdx : 23 , times: 2 , cards : [23,23,24] }
	DDZ_Frame_WaitTiLaChuai, // [ 0,1]
	DDZ_Frame_DoTiLaChuai, // { isTiLaChuai : 0 }
	DDZ_Frame_WaitChu, // { idx : 0 }
	DDZ_Frame_DoChu,  // { cards : [2,2,4,2 ], type : 2  }   // if key is null , means pass ; 
	DDZ_Frame_GameEnd, // { bombCnt: 2 , isChunTian : 0 , isMingPai : 1 , bottom : 2 , players : [{ idx: 2 , offset : -2,cards : [23,23,2] }, .....] }
	DDZ_Frame_Double, // { idx : 0, double : 2/0 }
};

enum DDZ_RotLandlordType
{
	eFALRLT_None,
	eFALRLT_Call = eFALRLT_None, //叫地主
	eFALRLT_Rot, //抢地主
	eFALRLT_Max,
};