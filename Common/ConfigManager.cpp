#include "ConfigManager.h"
#include <string>
#include "log4z.h"
#include "ShopConfg.h"
CConfigManager::CConfigManager()
{
	memset(m_vConfigs,0,sizeof(m_vConfigs)) ;
}

CConfigManager::~CConfigManager()
{
	for ( int i = eConfig_One; i < eConfig_Max ; ++i )
	{
		delete m_vConfigs[i] ;
		m_vConfigs[i] = NULL ;
	}
}

void CConfigManager::LoadAllConfigFile( const char* pConfigRootPath )
{
	LOGFMTI("load all config") ;

	std::string strCL = pConfigRootPath ;
	if (strCL.at(strCL.size() -1 ) != '/')
	{
		strCL.append("/");
	}
	std::string pConfgiPath[eConfig_Max] ;
	pConfgiPath[eConfig_Shop] = strCL + "ShopConfig.txt" ;
	// shop config 
	m_vConfigs[eConfig_Shop] = new CShopConfigMgr ;
	for ( int i = eConfig_One; i < eConfig_Max ; ++i )
	{
		if ( m_vConfigs[i] )
		{
			m_vConfigs[i]->LoadFile(pConfgiPath[i].c_str()) ;
		}
	}
}

IConfigFile* CConfigManager::GetConfig( eConfigType eConfig )
{
	if ( eConfig >= eConfig_Max || eConfig < eConfig_One )
		return NULL ;
	return m_vConfigs[eConfig] ;
}