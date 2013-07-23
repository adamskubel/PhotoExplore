#include "SwipeGestureDetector.hpp"
#include "TextPanel.h"
#include "GlobalConfig.hpp"
#include <boost/property_tree/ptree.hpp>
#include "LeapDebug.h"

using namespace boost::property_tree;

SwipeGestureDetector::SwipeGestureDetector()
{
	DrawingEnabled = GlobalConfig::tree()->get<bool>("Leap.CustomGesture.Swipe.DebugDraw");
	sampleCount = 300;
	infoText = new TextPanel("0____0");
	((TextPanel*)infoText)->setTextAlignment(1);
	state = IdleState;
	currentScrollVelocity = 0;
}

SwipeGestureDetector::~SwipeGestureDetector()
{
}

SwipeGestureDetector& SwipeGestureDetector::getInstance()
{
	static SwipeGestureDetector instance; 
	return instance;
}

void SwipeGestureDetector::setSwipeDetectedListener(boost::function<void(Hand swipingHand, Vector swipeVector)> _swipeDetectedListener)
{
	this->swipeDetectedListener = _swipeDetectedListener;
}

void SwipeGestureDetector::setFlyWheel(FlyWheel * _flyWheel)
{
	flyWheelMutex.lock();
	for (int i=0;i<scrollPointVisuals.size();i++)
		scrollPointVisuals.at(i)->size = 0;

	this->flyWheel = _flyWheel;
	swipeMap.clear();
	
	flyWheelMutex.unlock();
}


void SwipeGestureDetector::doGestureScrolling(const Controller & controller)
{	
	Frame frame = controller.frame();
	ptree swipeConfig = GlobalConfig::tree()->get_child("Leap.CustomGesture.Swipe");
	Vector gestureVector;		
	GestureList gestures; 
	if (lastFrame.isValid())
		gestures = frame.gestures(lastFrame);
	else
		gestures = frame.gestures();

	Hand gestureHand;
	for (auto it = gestures.begin(); it != gestures.end(); it++)
	{
		Gesture g = *it;

		if (g.state() == Gesture::STATE_INVALID || g.state() == Gesture::STATE_STOP)
			continue;
		if (g.type() != Gesture::TYPE_SWIPE)
			continue;		

		SwipeGesture swipe(g);


		if (!gestureHand.isValid())
			gestureHand = swipe.hands()[0];

		if (lastFrame.isValid() && gestureHand.isValid())
		{
			if (lastActiveTime.millis() > 100)
			{
				swipeMap.clear();
			}
			lastActiveTime.start();
						
			if (swipeMap.find(swipe.id()) == swipeMap.end())
			{
				swipeMap.insert(make_pair(swipe.id(),TrackedSwipe(swipe.id())));	
			}		

			swipeMap[swipe.id()].distanceList.push_back((swipe.startPosition()-swipe.position()).magnitude());
			if (swipeMap[swipe.id()].distanceList.size() > sampleCount)
				swipeMap[swipe.id()].distanceList.pop_front();

			Hand h1 = gestureHand;
			Hand h2 = lastFrame.hand(h1.id());

			if (h2.isValid())
			{
				float sphereAccel = h1.sphereRadius() -  h2.sphereRadius();

				if (sphereAccel > swipeMap[swipe.id()].maxSphereGrowth) 
				{
					swipeMap[swipe.id()].maxSphereGrowth = sphereAccel;
				}
				else if (sphereAccel < swipeMap[swipe.id()].minSphereGrowth) 
				{
					swipeMap[swipe.id()].minSphereGrowth  = sphereAccel;
				}
			}

			float pointableDistance = swipe.pointable().touchDistance();
			swipeMap[swipe.id()].touchDistanceList.push_back(pointableDistance*50.0f);
			if (swipeMap[swipe.id()].touchDistanceList.size() > sampleCount)
				swipeMap[swipe.id()].touchDistanceList.pop_front();
			if (pointableDistance < swipeMap[swipe.id()].minTouchDistance) 
			{
				swipeMap[swipe.id()].minTouchDistance = pointableDistance;
			}		

			
			swipeMap[swipe.id()].speedList.push_back(swipe.speed());
			if (swipeMap[swipe.id()].speedList.size() > sampleCount)
				swipeMap[swipe.id()].speedList.pop_front();
			
			float swipeSpeed = 0, sampleCount = 0, numSpeedSamples = 10;

			for (auto it = swipeMap[swipe.id()].speedList.begin(); it != swipeMap[swipe.id()].speedList.end() && sampleCount < numSpeedSamples; it++)
			{
				sampleCount++;
				swipeSpeed = max<float>(swipeSpeed,*it);
			}
			//swipeSpeed /= sampleCount;
			swipeSpeed *= swipeConfig.get<float>("SpeedScale");

			bool triggered = false;
			if (swipeConfig.get<bool>("SphereGrowth.Enabled"))
			{
				if (swipeMap[swipe.id()].maxSphereGrowth > swipeConfig.get<float>("SphereGrowth.MinPositive"))
				{
					triggered = true;
				}
				else if (swipeMap[swipe.id()].minSphereGrowth < swipeConfig.get<float>("SphereGrowth.MaxNegative"))
				{						
					triggered = true;
				}
			}

			if (!triggered && swipeConfig.get<bool>("TouchZoneDistance.Enabled"))
			{
				if (swipeMap[swipe.id()].minTouchDistance < swipeConfig.get<float>("TouchZoneDistance.MaxTouchDistance"))
				{						
					triggered = true;
				}
			}

			if (triggered)
			{
				state = GestureScrolling;
				flyWheel->setFriction(.5f);
				if (abs(swipeSpeed) > swipeConfig.get<float>("MaxSpeed"))
					swipeSpeed = sgn(swipeSpeed) * swipeConfig.get<float>("MaxSpeed");
								
				if (abs(swipeSpeed) < swipeConfig.get<float>("MinSpeed"))
					swipeSpeed = sgn(swipeSpeed) * swipeConfig.get<float>("MinSpeed");

				if (abs(swipeSpeed) > abs(flyWheel->getVelocity()))
				{					
					flyWheel->setVelocity((swipeSpeed*swipe.direction()).x);
				}
			}
		}

	}
	
	if (gestureHand.isValid() && DrawingEnabled)
	{
		stringstream ss;
		for (auto it = swipeMap.begin(); it != swipeMap.end(); it++)
		{		
			ss << it->first << "_Touch[" << it->second.minTouchDistance << "]_Sphere-[" << it->second.minSphereGrowth << "]_Sphere+[" << it->second.maxSphereGrowth << "]   ";		
		}
		((TextPanel*)infoText)->setText(ss.str());
	}
}


static Vector getCentroid(const Controller & controller, Hand hand)
{
	Vector centroid;
	float totalTouchDistance = 0;
	for (int i=0;i<hand.fingers().count(); i++)
	{
		Finger f = hand.fingers()[i];
		centroid += LeapHelper::FindScreenPoint(controller,f) * (2.0f - f.touchDistance());
		totalTouchDistance += 2.0f - f.touchDistance();
	}
	centroid /= totalTouchDistance;

	return centroid;
}


static int findClosestPointable(const Controller & controller, Hand hand)
{
	float minTouchDistance = 1;
	int minTouchPointable = -1;
	for (int i=0;i<hand.pointables().count(); i++)
	{
		Pointable p = hand.pointables()[i];
		if (p.touchDistance() < minTouchDistance)
		{
			minTouchDistance = p.touchDistance();
			minTouchPointable = p.id();
		}
	}
	return minTouchPointable;
}


void SwipeGestureDetector::doTouchZoneScrolling(const Controller & controller)
{	
	ptree scrollConfig = GlobalConfig::tree()->get_child("Leap.TouchScroll");

	Frame frame = controller.frame();

	static float maxFrictionInductionSpeed = scrollConfig.get<float>("FrictionInduce.MaxTipVelocity");
	static float frictionScale = scrollConfig.get<float>("FrictionInduce.FrictionScale");
	static float touchDistanceScale = scrollConfig.get<float>("FrictionInduce.TouchDistanceScale");
	static float touchDistanceFrictionThreshold = scrollConfig.get<float>("FrictionInduce.TouchDistanceThreshold");
	static float touchDistancePreScale = scrollConfig.get<float>("FrictionInduce.TouchDistancePreScale");


	static float flywheelVelocityThreshold = scrollConfig.get<float>("FlyWheelVelocityThreshold");
	static float maxContinueScrollDistance = scrollConfig.get<float>("MaxContinueScrollDistance");
	static float maxStartScrollDistance = scrollConfig.get<float>("MaxStartScrollDistance");
	static float flingVelocityTransferRatio = scrollConfig.get<float>("FlingVelocityTransferRatio");
	static double filterRC = scrollConfig.get<double>("ScrollPositionFilterRC");
	static int minScrollFingers = scrollConfig.get<int>("MinScrollFingers");	
	static float cursorDimension = (((float)GlobalConfig::ScreenWidth) / 2560.0f) *  GlobalConfig::tree()->get<float>("Overlay.BaseCursorSize");
	static bool drawIntentOnly = GlobalConfig::tree()->get<bool>("Leap.TouchDistance.DrawIntentOnly");
	static float maxFlingSpeed = scrollConfig.get<float>("MaxFlingSpeed");

	static Timer filterTimer;

	HandModel * handModel = HandProcessor::LastModel();

	Hand h1 = frame.hand(handModel->HandId);

	float tipVelocityFriction = 0;

	if (h1.isValid() && h1.fingers().count() >= minScrollFingers)
	{
		if (state == TouchScrolling)
		{
			flyWheel->setFriction(.5f);
			Pointable scrollingPointable = frame.pointable(scrollingPointableId);
			if (!scrollingPointable.isValid())
			{
				scrollingPointableId =  -1;
				Hand scrollingHand = frame.hand(scrollingHandId);
				if (scrollingHand.isValid())
				{
					scrollingPointable = frame.pointable(findClosestPointable(controller,scrollingHand));
					if (scrollingPointable.isValid() && scrollingPointable.touchDistance() < maxContinueScrollDistance)
					{
						startScrollScreenPoint = LeapHelper::FindScreenPoint(controller, scrollingPointable);
						startScrollPos = flyWheel->getCurrentPosition();
						scrollingPointableId = scrollingPointable.id();
					}
				}

				if (scrollingPointableId < 0)
				{
					flyWheel->setVelocity(currentScrollVelocity*flingVelocityTransferRatio);
					state = IdleState;
				}
			}
			else if (scrollingPointable.touchDistance() < maxContinueScrollDistance)
			{
				Vector screenPoint = LeapHelper::FindScreenPoint(controller,scrollingPointable);
				Vector scrollBy = screenPoint - startScrollScreenPoint;				

				double newPos = LeapHelper::lowpass(flyWheel->getCurrentPosition(),startScrollPos + scrollBy.x,filterRC,filterTimer.seconds());
				currentScrollVelocity = (newPos - flyWheel->getCurrentPosition())/filterTimer.seconds();
				filterTimer.start();
			
				flyWheel->overrideValue(newPos);
			}
			else
			{
				float flingSpeed = currentScrollVelocity*flingVelocityTransferRatio;

				if (abs(flingSpeed) > maxFlingSpeed)
					flingSpeed = sgn(flingSpeed)*maxFlingSpeed;
				
				flyWheel->setVelocity(flingSpeed);
				state = IdleState;
			}
		}
		else 
		{
			float avgTipVelocity = 0;
			float minTouchDistance = 1;
			int minTouchPointable = -1;
			for (int i=0;i<h1.fingers().count(); i++)
			{
				Finger f = h1.fingers()[i];
				avgTipVelocity += f.tipVelocity().magnitude();
				if (f.touchDistance() < minTouchDistance)
				{
					minTouchDistance = f.touchDistance();
					minTouchPointable = f.id();
				}
			}
			avgTipVelocity /= ((float)h1.fingers().count());

			if (abs(flyWheel->getVelocity()) < flywheelVelocityThreshold && minTouchDistance < maxStartScrollDistance)
			{				
				Pointable scrollingPointable = frame.pointable(minTouchPointable);

				if (scrollingPointable.isValid())
				{
					startScrollScreenPoint = LeapHelper::FindScreenPoint(controller,scrollingPointable);
					startScrollPos = flyWheel->getCurrentPosition();
					scrollingPointableId = minTouchPointable;
					scrollingHandId = frame.pointable(scrollingPointableId).hand().id();
					state = TouchScrolling;
				}
			}
			else if (abs(flyWheel->getVelocity()) > flywheelVelocityThreshold && avgTipVelocity < maxFrictionInductionSpeed)
			{
				tipVelocityFriction = (maxFrictionInductionSpeed - avgTipVelocity)/maxFrictionInductionSpeed;
				tipVelocityFriction *= frictionScale;
				if (minTouchDistance < touchDistanceFrictionThreshold)
					tipVelocityFriction += powf(touchDistancePreScale * (touchDistanceFrictionThreshold - minTouchDistance),2.0f)*touchDistanceScale;

				tipVelocityFriction = max<float>(tipVelocityFriction,.5f);
				flyWheel->setFriction(tipVelocityFriction);
			}
		}
	}
	else if (state == TouchScrolling)
	{
		float flingSpeed = currentScrollVelocity*flingVelocityTransferRatio;

		if (abs(flingSpeed) > maxFlingSpeed)
			flingSpeed = sgn(flingSpeed)*maxFlingSpeed;
		
		flyWheel->setVelocity(flingSpeed);
		state = IdleState;
	}


	for (int i=0;i<scrollPointVisuals.size();i++)
		scrollPointVisuals.at(i)->size = 0;

	for (int i=0;i<h1.pointables().count(); i++)
	{
		Pointable p = h1.pointables()[i];

		if (scrollPointVisuals.size() <= i)
		{
			LeapDebugVisual * scrollPointVisual = new LeapDebugVisual(Vector(),1,LeapDebugVisual::LiveForever,0,Color(scrollConfig.get_child("VisualBackgroundColor")));
			scrollPointVisual->lineColor = Color(scrollConfig.get_child("VisualLineColor"));
			scrollPointVisual->depth = 10;
			LeapDebug::instance->addDebugVisual(scrollPointVisual);
			scrollPointVisuals.push_back(scrollPointVisual);
		}

		if (drawIntentOnly && p.id() != handModel->IntentFinger) continue;
		
		if (state == TouchScrolling) // || tipVelocityFriction > 0)
		{
			scrollPointVisuals.at(i)->size = cursorDimension;
			scrollPointVisuals.at(i)->screenPoint = LeapHelper::FindScreenPoint(controller,p);		
			
			//Color bg = Color(scrollConfig.get_child("VisualBackgroundColor"));
			//if (tipVelocityFriction > 0)
			//{
			//	bg.setAlpha(min<float>(1.0f,tipVelocityFriction/2.0f)*bg.colorArray[3]);
			//}
			//scrollPointVisuals.at(i)->fillColor = bg;
		} 
		else
		{			
			scrollPointVisuals.at(i)->size = 0;
		}
	}

}


void SwipeGestureDetector::onFrame(const Controller & controller)
{
	if (flyWheelMutex.try_lock())
	{
		Frame frame = controller.frame();
		if (frame.id() == lastFrame.id())
		{
			flyWheelMutex.unlock();
			return;
		}
		if (flyWheel == NULL)
		{
			lastFrame = frame;
			flyWheelMutex.unlock();
			return;
		}

		doTouchZoneScrolling(controller);
	
		if (state != TouchScrolling)
			doGestureScrolling(controller);

	
		lastFrame = frame;
		flyWheelMutex.unlock();
	}
}

void SwipeGestureDetector::draw()
{
	if (DrawingEnabled) 
	{
		static float maxV = GlobalConfig::ScreenHeight*.2f;
		
		static Color colors [] = {Colors::Red,Colors::Blue,Colors::Orange,Colors::HoloBlueBright,Colors::LeapGreen};

		glBindTexture(GL_TEXTURE_2D,NULL);

		float originY = 200;

		glColor4fv(Colors::Red.getFloat());
		glLineWidth(1);
		glBegin(GL_LINE_STRIP);
		glVertex3f(0,originY,11);
		glVertex3f(GlobalConfig::ScreenWidth,originY,11);
		glEnd();
		
		int count = 0;
		for (auto it = swipeMap.begin(); it != swipeMap.end();it++)
		{
			glColor4fv(colors[it->first%5].getFloat());
			glLineWidth(3);
			float x = 0;

			glBegin(GL_LINE_STRIP);
			for (auto point = it->second.distanceList.begin(); point != it->second.distanceList.end(); point++)
			{
				float xD = ((x++)*(GlobalConfig::ScreenWidth/sampleCount)*.4f) + 200;
				float yD = (*point)+originY+count;
				glVertex3f(xD,yD,10);
			}
			glEnd();

			x = (GlobalConfig::ScreenWidth/sampleCount)*.45f;

			glBegin(GL_LINE_STRIP);
			for (auto point = it->second.touchDistanceList.begin(); point != it->second.touchDistanceList.end(); point++)
			{
				float xD = ((x++)*(GlobalConfig::ScreenWidth/sampleCount)*.4f) + 200;
				float yD = (*point)+originY+count;
				glVertex3f(xD,yD,10);
			}
			glEnd();

			if (x != 0)
			{
				count+= GlobalConfig::ScreenHeight*.22f;
				if (count > GlobalConfig::ScreenHeight*.7f)
					count = 0;
			}
		}
		infoText->layout(Vector(300,GlobalConfig::ScreenHeight*.6f,10), cv::Size2f(600,400));
		infoText->draw();
	}

}