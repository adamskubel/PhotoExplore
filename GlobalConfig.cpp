#include "GlobalConfig.hpp"
#include <fstream>

int GlobalConfig::ScreenWidth = 1920;
int GlobalConfig::ScreenHeight = 1080;
bool GlobalConfig::LeftHanded = false;
std::string GlobalConfig::TestingToken = std::string("");



GlobalConfig::GlobalConfig()
{
	propertyFileLoaded = false;
}

GlobalConfig& GlobalConfig::getInstance()
{
	static GlobalConfig instance;
	return instance;
}

bool GlobalConfig::isLoaded()
{
	return propertyFileLoaded;
}


void GlobalConfig::loadConfigFile(string path)
{
	using boost::property_tree::ptree;
	
	std::ifstream inf;
	inf.open(path,std::ios::in);
	
	boost::property_tree::read_json(inf,getInstance().propertyTree);
	propertyFileLoaded = true;
	inf.close();

}

//void GlobalConfig::mergeConfigFile(string path)
//{
//	using boost::property_tree::ptree;
//	
//	std::ifstream inf;
//	inf.open(path,std::ios::in);
//	
//	ptree newTree;
//	
//	boost::property_tree::read_json(inf,newTree);
//	inf.close();
//	
//	
//	
//}

boost::property_tree::ptree * GlobalConfig::tree()
{
	if (!getInstance().isLoaded())
		throw new std::runtime_error("uh oh");
	return &(getInstance().propertyTree);
}