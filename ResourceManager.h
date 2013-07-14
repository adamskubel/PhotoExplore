#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_


#include <set>
#include <queue>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <boost/thread/mutex.hpp>

#include "ResourceManagerTypes.h"
#include <opencv2/opencv.hpp>


struct IdIndex{};

using namespace std;


struct CachedResource
{
	ResourceData * Data;

	string resourceId;

	CachedResource(ResourceData * _data) :
		Data(_data),		
		resourceId(_data->resourceId)
	{
	}
};


typedef boost::multi_index_container
	<
		CachedResource,
		boost::multi_index::indexed_by
		<
			boost::multi_index::hashed_unique
			<
				boost::multi_index::tag<IdIndex>,
				boost::multi_index::member<CachedResource,string,&CachedResource::resourceId>
			>
		>
	> ResourceCache;



class ResourceManager {

private:	
	ResourceManager();
	ResourceManager(ResourceManager const&);
	void operator=(ResourceManager const&); 

	ResourceCache resourceCache;
	bool resourcesChanged;
	//queue<Resource*> modifiedResources;
	boost::mutex resourceMutex;

	boost::mutex updateTaskMutex;
	queue<boost::function<void()> > updateThreadTasks;
	

	float maxTextureLoadPriority;
	float maxImageLoadPriority;

	void cleanupCache();
	
	void textureLoaded(ResourceData * data, GLuint textureId, int taskStatus);

	float textureLoadThreshold, imageLoadThreshold;

	void updateImageState(ResourceData * data, bool load);	
	void updateTextureState(ResourceData * data, bool load);

	void updateImageResource(string resourceId, int statusCode, cv::Mat imgMat);

public:
		
	static ResourceManager& getInstance()
	{
		static ResourceManager instance; 
		return instance;
	}
	

	ResourceData * loadResource(string resourceId, string imageURI, float priority, IResourceWatcher * watcher);	
	ResourceData * loadResource(string resourceId, cv::Mat & image, float priority, IResourceWatcher * watcher);
	
	void updateResource(ResourceData * resource);

	void update();

};


#endif