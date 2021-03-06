#include "Flywheel.h"
#include "LeapHelper.h"



FlyWheel::FlyWheel(double startPosition)
{
	minVelocity = INT_MIN;
	minValue = INT_MIN;
	maxVelocity = INT_MAX;
	maxValue = INT_MAX;
	friction = .5;
	velocity = 0;
	position = startPosition;
	wheelTimer.start();
	boundaryMode = BounceBack;
	hasTarget = false;
}

double FlyWheel::getMaxValue()
{
	return maxValue;
}

void FlyWheel::setMaxValue(double maxValue)
{
	this->maxValue = maxValue;
}

double FlyWheel::getMinValue()
{
	return minValue;
}

void FlyWheel::setMinValue(double minValue)
{
	if (minValue < maxValue)
		this->minValue = minValue;
	else
		this->minValue = maxValue;
}

void FlyWheel::overrideValue(double newValue)
{	
	velocity = 0;
	this->position = newValue;
}


void FlyWheel::setDraggingState(bool _isDragging)
{
	this->isDragging = _isDragging;
}


void FlyWheel::setFriction(double f)
{
	friction = f;
}

double FlyWheel::getFriction()
{
	return friction;
}


FlyWheel::BoundaryMode FlyWheel::getBoundaryMode()
{
	return boundaryMode;
}

void FlyWheel::setBoundaryMode(FlyWheel::BoundaryMode _mode)
{
	this->boundaryMode = _mode;
}


void FlyWheel::setTargetActive(bool targetActive)
{
	this->hasTarget = targetActive;
}

void FlyWheel::setTargetPosition(double _targetPosition)
{
	this->targetPosition = _targetPosition;

	targetPosition = min<volatile double>(maxValue,targetPosition);
	targetPosition = max<volatile double>(minValue,targetPosition);
}

double FlyWheel::getTargetPosition()
{
	return targetPosition;
}

void FlyWheel::boundPosition()
{	
	static float max_Kp = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.MaxBoundLimiter.Proportional");
	static float max_alpha = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.MaxBoundLimiter.VelocityComponent");

	static float min_Kp = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.MinBoundLimiter.Proportional");
	static float min_alpha = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.MinBoundLimiter.VelocityComponent");

	if (boundaryMode == BounceBack)
	{
		if (max_Kp > 0 && min_Kp > 0)
		{
			if (position > maxValue)
			{
				velocity =  velocity*max_alpha +  (max_Kp  * (maxValue-position))*(1.0f - max_alpha);
			}
			else if (position < minValue)
			{
				velocity =  velocity*min_alpha + (min_Kp  * (minValue-position))*(1.0f - min_alpha);
			}
		}
		else
		{
			if (position > maxValue)
			{
				position = maxValue;
				if (velocity > 0)
					velocity = 0;
			}
			else if (position <= minValue)
			{
				position = minValue;
				if (velocity < 0)
					velocity = 0;
			}  
		}
	}
	else if (boundaryMode == WrapAround)
	{
		if (position > maxValue)
		{
			double d = position-maxValue;

			position = minValue + d;
		}
		else if (position <= minValue)
		{
			double d = position-minValue;
			position = maxValue+d;
		}  
	}
}

void FlyWheel::update(double elapsedSeconds)
{	
	static float targetKp = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.TargetSeeker.Proportional");
	static float targetAlpha = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.TargetSeeker.VelocityComponent");
	static float maxTargetVelocity = GlobalConfig::tree()->get<float>("ScrollView.FlyWheel.TargetSeeker.MaxVelocity");

	if (abs(velocity) > maxVelocity)
	{
		velocity = sgn(velocity) * maxVelocity;
	}


	if (velocity != 0)
	{
		double frictionAccel;
		frictionAccel = (-velocity * friction);

		velocity += frictionAccel * elapsedSeconds;
		position += (velocity * elapsedSeconds);
	}


	if (hasTarget)
	{
		velocity =  velocity*targetAlpha +  (targetKp  * (targetPosition-position))*(1.0f - targetAlpha);

		if (abs(velocity) > maxTargetVelocity)
		{
			velocity = sgn(velocity) * maxTargetVelocity;
		}
	}

	boundPosition();	
}

void FlyWheel::flingWheel(double flingVelocity)
{
	if (abs(flingVelocity) > abs(getVelocity()))
	{				
		setFriction(.5f);	
		setVelocity(flingVelocity);
	}
}

void FlyWheel::impartVelocity(double _velocity)
{
	setVelocity(_velocity);
}

double FlyWheel::getVelocity()
{
	return velocity;
}

void FlyWheel::setVelocity(double _velocity)
{
	this->velocity = _velocity;
}

double FlyWheel::getCurrentPosition()
{
	return position;
}

double FlyWheel::getPosition()
{	
	update(wheelTimer.seconds());
	wheelTimer.start();
	return position;
}