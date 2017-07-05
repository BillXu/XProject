#pragma once
class IConfigFile ;
class CContiuneLoginConfigMgr ;
class CTitleLevelConfig;

class CConfigManager
{
public:
	enum eConfigType
	{
		eConfig_One,
		eConfig_Shop = eConfig_One,
		eConfig_Max,
	};
public:
	CConfigManager();
	~CConfigManager();
	void LoadAllConfigFile( const char* pConfigRootPath );
	IConfigFile* GetConfig( eConfigType eConfig );
protected:
	IConfigFile* m_vConfigs[eConfig_Max] ;
};