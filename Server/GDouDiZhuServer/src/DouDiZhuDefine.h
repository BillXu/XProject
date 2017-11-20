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
};