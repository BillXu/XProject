#pragma once
enum eGoldenActType
{
	eGoldenAct_Call, //¸ú×¢
	eGoldenAct_AddCall, //¼Ó×¢
	eGoldenAct_PK, //±ÈÅÆ
	eGoldenAct_Look, //¿´ÅÆ
	eGoldenAct_CallToEnd, //Ò»¸úµ½µ×
	eGoldenAct_Pass, //ÆúÅÆ
};

enum eGoldenFrameType
{
	eGoldenFrame_StartGame,// { bankIdx : 2, players : [ { uid : 23,idx : 0, coin : 23 , cards : [1,3,4,5] }, .... ] }
	eGoldenFrame_WaitPlayerAct,  // { idx : 2, acts: [ act : eAct_Pass , info : 0 ] , [ act : 1 , info : 0 ] ... ] }
	eGoldenFrame_Pass, // { idx : 2 }
	eGoldenFrame_Call, // { idx : 0 , coin : 10 }
	eGoldenFrame_Look, // { idx : 0 }
	eGoldenFrame_PK, // { idx : 0 , withIdx : 1 , result : 1}
	eGoldenFrame_END_PK, // { participate : { 1 , 2 , 3 } , lose : { 1 , 2 } }
};