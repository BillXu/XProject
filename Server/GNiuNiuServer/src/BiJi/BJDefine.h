#pragma once
enum eBJCardType
{
	CardType_None,  // common 
	CardType_Pair,  // dui zi 
	CardType_Sequence, // shun zi
	CardType_SameColor,  // jin hua
	CardType_SameColorSequence, // shun jin
	CardType_Bomb,  // bao zi 
	CardType_Max,
};

enum eXiPaiType
{
	eXiPai_None,
	eXiPai_AllBlack = eXiPai_None,
	eXiPai_AllRed,
	eXiPai_QuanShun,
	eXiPai_ShuangShunQing,
	eXiPai_ShuangSanTiao,
	eXiPai_SiZhang,
	eXiPai_ShuangSiZhang,
	eXiPai_QuanShunQing ,
	eXiPai_QuanSanTiao,
	eXiPai_SanQing,
	eXiPai_ShunQingDaTou,
	eXiPai_Max,
};