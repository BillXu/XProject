#pragma once
enum GD_Type
{
	GD_None,
	GD_Single, //����
	GD_Pair, //����
	GD_3Pices, //3��
	GD_3Follow2, //3��2
	GD_SingleSequence5, //5����˳
	GD_PairSequence3, //3����˳
	GD_3PicesSeqence2, //�ְ�
	GD_Common,
	GD_Bomb4Or5, //4�Ż�5��ը��
	GD_Flush5, //ͬ��˳
	GD_Bomb6OrMore, //6�ż�����ը��
	GD_Rokect, //4����
	GD_Max,
};

enum GD_FRAME
{
	GD_Frame_WaitChaoZhuang, // [ { idx : 2, uid : 23 }, .... ]
	GD_Frame_DoChaoZhuang, // { idx : 0 }
	GD_Frame_StartGame, // [ { idx : 0 , uid : 23, cards : [23,23,23] }, .... ]
	GD_Frame_WaitRobBanker,// { idx : 2 } 
	GD_Frame_DoRobBanker, // { idx : 2 , times : 1 }
	GD_Frame_DoProducedBanker, // { dzIdx : 23 , times: 2 , cards : [23,23,24] }
	GD_Frame_WaitTiLaChuai, // [ 0,1]
	GD_Frame_DoTiLaChuai, // { isTiLaChuai : 0 }
	GD_Frame_WaitChu, // { idx : 0 }
	GD_Frame_DoChu,  // { cards : [2,2,4,2 ], type : 2  }   // if key is null , means pass ; 
	GD_Frame_GameEnd, // { bombCnt: 2 , isChunTian : 0 , isMingPai : 1 , bottom : 2 , players : [{ idx: 2 , offset : -2,cards : [23,23,2] }, .....] }
	GD_Frame_Double, // { idx : 0, double : 2/0 }
	GD_Frame_PayTribute, // {info : { {idx : 0, target : 1, card : 123}, {}, ... }}
	GD_Frame_BackTribute, // {info : { {idx : 0, target : 1, card : 123}, {}, ... }}
	GD_Frame_Daji, // { daji : 2 }
};