#include "TextureLoadTask.hpp"		

#include "GLImport.h"
#include "SDLTimer.h"
#include <math.h>
#include "TexturePool.h"
#include "PixelBufferPool.hpp"
#include "Logger.hpp"
#include "LeapDebug.h"

using namespace std;
using namespace LoadTaskState;

//#define  MaxBytesPerFrame 1048576

int TextureLoadTask::GlobalTaskId = 0;

#define TEXTURE_LOGGING true

TextureLoadTask::TextureLoadTask(std::string _resourceId, float _priority, cv::Mat _cvImage, boost::function<void(GLuint textureId, int taskStatus)> _callback):
	cvImage(_cvImage),
	priority(_priority),
	resourceId(_resourceId),
	callback(_callback)
{
	MaxBytesPerFrame = (int)(GlobalConfig::tree()->get<double>("GraphicsSettings.TextureLoading.MaxMBPerFrame")*1048576.0);
	
	useCompression = true; // GlobalConfig::tree()->get<bool>("GraphicsSettings.TextureLoading.Compression.Enable");
	timeMultiplexBufferToTextureTransfer = GlobalConfig::tree()->get<bool>("GraphicsSettings.TextureLoading.BufferToTexture.TimeMultiplex");

	if (cvImage.data == NULL || cvImage.size().area() == 0)
		throw new std::runtime_error("Invalid image");

	sourceBuffer = NULL;
	textureInfo.textureId = NULL;
	state = New;
	taskId = GlobalTaskId++;
}


TextureLoadTask::~TextureLoadTask()
{
	//cleanup();
}

void TextureLoadTask::setPriority(float _priority)
{
	this->priority = _priority;
}

GLuint TextureLoadTask::getTextureId()
{
	return textureInfo.textureId;
}

int TextureLoadTask::getState()
{
	return state;
}

void TextureLoadTask::cancel()
{	
	if (!active)
	{
		if (textureInfo.textureId != NULL)
		{
			TexturePool::getInstance().releaseTexture(textureInfo.textureId);
			textureInfo.textureId = NULL;
		}

		state = Error;
		cleanup();
	}
}


void TextureLoadTask::update()
{
	switch (state)
	{
	case New:
		startTask();
		break;
	case Initialized:
		initializeTexture();
		break;
	case LoadingBuffer:
		copyMemoryToBuffer_async_update();
		break;
	case BufferLoaded:
		if (timeMultiplexBufferToTextureTransfer)
			copyBufferToTexture_TM_start();
		else
			copyBufferToTexture();
		break;
	case LoadingTexture:
		copyBufferToTexture_TM_update();
		break;
	case Waiting:
		cleanup();	
		state = Complete;
		break;
	case Complete:				
		break;
	case Error:
		break;
	}
}



void TextureLoadTask::startTask()
{
	if (textureInfo.textureId == NULL)
	{
		GLuint textureId = TexturePool::getInstance().getTexture();
		if (textureId != NULL)
			textureInfo = TextureInfo(textureId,GL_RGBA,4,cvImage.size().width,cvImage.size().height);
		else
			return;
	}

	if (PixelBufferPool::getInstance().isEnabled())
	{
		sourceBuffer = PixelBufferPool::getInstance().getBuffer();
		if (sourceBuffer != NULL)
		{
			Logger::stream("TextureLoadTask","INFO") << "Load task started. TaskId = " << taskId << " TexID = " << textureInfo.textureId << " BufferID = " << sourceBuffer << endl;
			state = Initialized;
			update();
		}
	}
	else if (useCompression)
	{
		glBindTexture(GL_TEXTURE_2D, textureInfo.textureId);
		glTexImage2D(GL_TEXTURE_2D,0, GL_COMPRESSED_RGBA_ARB, textureInfo.width,textureInfo.height,0,textureInfo.format,GL_UNSIGNED_BYTE,cvImage.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		
		state = Complete;
		
		int result;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED_ARB,&result);
		int size;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB,&size);
		
		double ds = textureInfo.width*textureInfo.height*4.0/1048576.0;
		double cs = ((double)size)/1048576.0;
		
		glBindTexture(GL_TEXTURE_2D, NULL);
		Logger::stream("TextureLoadTask","INFO") << "Texture compressed. State = " << result << ". Compresssed by " << (cs/ds) << " to " << cs << " from " << ds << endl;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, textureInfo.textureId);
		glTexImage2D(GL_TEXTURE_2D,0,textureInfo.bytesPerPixel,textureInfo.width,textureInfo.height,0,textureInfo.format,GL_UNSIGNED_BYTE,cvImage.data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, NULL);
		state = Complete;
	}
}


void TextureLoadTask::initializeTexture()
{
	static double totalTime;
	static int notifyCount;
	static Timer initTimer;
	
	initTimer.start();
	glBindTexture(GL_TEXTURE_2D, textureInfo.textureId);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,textureInfo.width,textureInfo.height,0,textureInfo.format,GL_UNSIGNED_BYTE,0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	float color [] = {1.0f,1.0f,1.0f,0.0f};
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,&color[0]);
	glBindTexture(GL_TEXTURE_2D, NULL);
		
	OpenGLHelper::LogOpenGLErrors("TextureLoadTask-initializeTexture");

	//LeapDebug::out << "[" << Timer::frameId << "] Initialized TEX[" << textureInfo.textureId << "] in " << initTimer.millis() << "ms \n";

	if (TEXTURE_LOGGING)
	{
		if (initTimer.millis() > 2)
			Logger::stream("TextureLoadTask","INFO") << "Initialized texture. Took " << initTimer.millis() << " ms. TexID = " << textureInfo.textureId << " W= " << textureInfo.width << " H = " << textureInfo.height << endl;
	}

	dataLoaded = 0;
	copyMemoryToBuffer_async_start();
}

void TextureLoadTask::copyBufferToTexture_TM_update()
{
	
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);
	glBindTexture(GL_TEXTURE_2D, textureInfo.textureId);
	
	int loadRows = 100;
	int loadHeight = min<int>(textureInfo.height,loadedRows+loadRows);
	loadHeight -= loadedRows;
	
	glTexSubImage2D(GL_TEXTURE_2D,0,0,loadedRows, textureInfo.width,loadHeight,textureInfo.format,GL_UNSIGNED_BYTE, (const GLvoid*)(loadedRows*textureInfo.width*4));
	loadedRows += loadHeight;

	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,NULL);
	
	if (loadedRows == textureInfo.height)
	{
		state = Waiting;
	}
}

void TextureLoadTask::copyBufferToTexture_TM_start()
{
	loadedRows = 0;
	state  = LoadingTexture;
	//copyBufferToTexture_TM_update();
}

void TextureLoadTask::copyBufferToTexture()
{
	static Timer timer;
	timer.start();
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);
	glBindTexture(GL_TEXTURE_2D, textureInfo.textureId);
	
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0, textureInfo.width,textureInfo.height,textureInfo.format,GL_UNSIGNED_BYTE, 0);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,NULL);
	
	//if (TEXTURE_LOGGING)
	{
		LeapDebug::getInstance().plotValue("PBO",Colors::Blue,timer.millis()*20);
		//if (timer.millis() > 2)
			//Logger::stream("TextureLoadTask","INFO") << "Copied to texture. TexID = " << textureInfo.textureId << " Took " << timer.millis() << " ms" << endl;
	}
	
	state  = Waiting;
}

void TextureLoadTask::copyMemoryToBuffer_async_start()
{
	Timer memTimer;
	memTimer.start();
	
	unsigned char * data = cvImage.data;
	
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, textureInfo.size, NULL, GL_STREAM_DRAW_ARB);
	GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL);
	
	if(ptr && data)
	{
		state = LoadingBuffer;
		Logger::stream("TextureLoadTask","DEBUG") << "Render thread: async_memory_copy - TexID = " << this->textureInfo.textureId << " bufferId = " << this->sourceBuffer << " taskId = " << this->taskId << endl;
		boost::thread([this,data,ptr](){
			this->active = true;
			Logger::stream("TextureLoadTask","DEBUG") << "Load thread: async_memory_copy - TexID = " << this->textureInfo.textureId << " bufferId = " << this->sourceBuffer << " taskId = " << this->taskId << endl;
			memcpy(ptr,data,this->textureInfo.size);
			this->active = false;
		});		
	}
	else
	{
		Logger::stream("TextureLoadTask","ERROR") << "copyMemoryToBuffer_async_start - Couldn't access pixel buffer. TaskId = " << taskId << " TexID = " << textureInfo.textureId << " BufferID = " << sourceBuffer << endl;
		cancel();
	}
	if (TEXTURE_LOGGING && memTimer.millis() > 2)
		Logger::stream("TextureLoadTask","TIME") << "Initialized buffer in " << memTimer.millis() << " ms" << endl;
}

void TextureLoadTask::copyMemoryToBuffer_async_update()
{	
	if (!active)
	{
		Logger::stream("TextureLoadTask","DEBUG") << "async_memory_update - TexID = " << this->textureInfo.textureId << " bufferId = " << this->sourceBuffer << " taskId = " << this->taskId << endl;

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);
		glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,NULL);
		
		state = BufferLoaded;
		OpenGLHelper::LogOpenGLErrors("TextureLoadTask-copyMemoryToBuffer_async_update");
	}
}

void TextureLoadTask::copyMemoryToBuffer_TM_update()
{	
	Timer memTimer;
	memTimer.start();

	int loadCount = (textureInfo.size - dataLoaded);

	loadCount = min<int>(loadCount,MaxBytesPerFrame);

	memcpy(loading_target+dataLoaded,loading_source+dataLoaded,loadCount);	

	dataLoaded += loadCount;

	if (dataLoaded >= textureInfo.size)
	{
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);  	
		glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); 			
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,NULL);

		state = BufferLoaded;
		OpenGLHelper::LogOpenGLErrors("TextureLoadTask-copyMemoryToBuffer_TM_update");
	}
	if (TEXTURE_LOGGING && memTimer.millis() > 2)
		Logger::stream("TextureLoadTask","INFO") << "TimeMultiplexed update took " << memTimer.millis() << " ms " << endl;
	

	//LeapDebug::out  << "[" << Timer::frameId << "] - TEX["<< textureInfo.textureId << "] from MEM to PBO[" << sourceBuffer << "]  in " << memTimer.millis() << "ms, @ " << ((loadCount/1048576)/memTimer.seconds()) << " MB/s \n";	
}

void TextureLoadTask::copyMemoryToBuffer_TM_start()
{
	Timer memTimer;
	memTimer.start();

	unsigned char * data = cvImage.data;

	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, sourceBuffer);  			
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, textureInfo.size, NULL, GL_STREAM_DRAW_ARB);
	GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, NULL);

	if(ptr && data)
	{				
		loading_source = data;
		loading_target = ptr;
		dataLoaded = 0;
		state = LoadingBuffer;
	}
	else
	{
		Logger::stream("TextureLoadTask","ERROR") << "copyMemoryToBuffer_TM_start - Couldn't access pixel buffer. TaskId = " << taskId << " TexID = " << textureInfo.textureId << " BufferID = " << sourceBuffer << endl;
		cancel();
	}
	if (TEXTURE_LOGGING && memTimer.millis() > 2)
		Logger::stream("TextureLoadTask","TIME") << "Initialized buffer in " << memTimer.millis() << " ms" << endl;
}


void TextureLoadTask::cleanup()
{
	if (sourceBuffer != NULL)
	{
		if (TEXTURE_LOGGING)
			Logger::stream("TextureLoadTask","INFO") << "Cleaning up. TaskId = " << taskId << " TexID = " << textureInfo.textureId << " BufferID = " << sourceBuffer << endl;

		PixelBufferPool::getInstance().freeBuffer(sourceBuffer);
		sourceBuffer = NULL;
	}
}