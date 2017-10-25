#pragma once
enum ePokerType
{
	ePoker_None,
	ePoker_Diamond = ePoker_None, // fangkuai
	ePoker_Club, // cao hua
	ePoker_Heart, // hong tao
	ePoker_Sword, // hei tao 
	ePoker_NoJoker,
	ePoker_Joker = ePoker_NoJoker,
	ePoker_Max,
};