#include "PanelBase.h"
#include"GraphicContext.hpp"

PanelBase::PanelBase()
{
	layoutDuration = -1;
	width = height = 0;
	borderThickness = 0;
	borderColor = Colors::Transparent;
	visible = true;
	NudgeAnimationEnabled = false;
	BorderSelectionThickness = 0;
	beforeSelectionBorderThickness = 0;
	ScaleOnEnter = 1;
	panelId = "";
	this->backgroundColor = Colors::Transparent;
	animateOnLayout = true;
	drawingLoadAnimation = false;
	loadAnimationColor = Colors::DimGray.withAlpha(.3f);

	desiredSize = cv::Size2f(0,0);
	useLineBorder = false;
}

void PanelBase::setBackgroundColor(Color color)
{
	this->backgroundColor = color;
}

Color PanelBase::getBackgroundColor()
{
	return this->backgroundColor;
}

void PanelBase::setBorderColor(Color color)
{
	this->borderColor = color;
}

void PanelBase::setBorderThickness(float b)
{
	this->borderThickness = b;
	beforeSelectionBorderThickness = b;
}

void PanelBase::setUseLineBorder(bool _useLineBorder)
{
	this->useLineBorder = _useLineBorder;
}

bool PanelBase::isUseLineBorder()
{
	return this->useLineBorder;
}


Vector PanelBase::getPosition()
{
	return position;
}

Vector PanelBase::getCenter()
{
	return position + Vector(width*.5f,height*.5f,0);
}

float PanelBase::getWidth()
{
	return width;
}

float PanelBase::getHeight()
{
	return height;
}

void PanelBase::requestLayout()
{
	//panelDirty = true;
}

void PanelBase::setAnimateOnLayout(bool _animateOnLayout)
{
	this->animateOnLayout = _animateOnLayout;
}


void PanelBase::setLayoutDuration(long _layoutDuration)
{
	this->layoutDuration = _layoutDuration;
}

bool PanelBase::isAnimateOnLayout()
{
	return this->animateOnLayout;
}

void PanelBase::measure(cv::Size2f & measuredSize)
{
	if (desiredSize.width == 0 || desiredSize.height == 0)
	{
		measuredSize.width = getWidth();
		measuredSize.height = getHeight();
	}
	else
		measuredSize = desiredSize;
}

void PanelBase::layout(Vector layoutPosition, cv::Size2f layoutSize)
{
	static long animationTime = GlobalConfig::tree()->get<long>("PanelBase.AnimateLayoutDuration");

	if (layoutDuration <= 0)
		layoutDuration = animationTime;

	this->lastSize = layoutSize;
	this->lastPosition = layoutPosition;

	bool found;
	float skipAnim;
	skipAnim = GraphicsContext::getInstance().getDrawHint("SkipLayoutAnimation",found);
	if (!found)
		skipAnim = 0;

	if (this->width == 0 && this->height == 0 || !isAnimateOnLayout() || skipAnim == 1)
	{
		setPosition(layoutPosition);
		setPanelSize(layoutSize.width,layoutSize.height);
	}
	else
	{
		animateToPosition(layoutPosition,animationTime,layoutDuration);
		animatePanelSize(layoutSize.width,layoutSize.height,layoutDuration);
	}
}


void PanelBase::setPanelSize(float width, float height, float padding)
{
	this->width = width;
	this->height = height;
	this->lastSize = cv::Size2f(width,height);
	//panelDirty = true;
}

void PanelBase::animatePanelSize(cv::Size2f targetSize, long duration)
{
	animatePanelSize(targetSize.width, targetSize.height,duration);
}

void PanelBase::animatePanelSize(float targetWidth, float targetHeight, long duration)
{
	
	if (duration <= 0)
		duration = sqrtf(powf(width-targetWidth,2) + pow(height-targetHeight,2));

	float startWidth = width,startHeight = height;
	if (widthAnimation.isRunning())
	{
		startWidth = widthAnimation.getValue();
		widthAnimation = DoubleAnimation(startWidth,targetWidth,duration-widthAnimation.getElapsedTime(), NULL);
	}
	else
	{
		widthAnimation = DoubleAnimation(startWidth,targetWidth,duration, NULL);
	}


	if (heightAnimation.isRunning())
	{
		startHeight = heightAnimation.getValue();
		heightAnimation = DoubleAnimation(startHeight,targetHeight,duration-heightAnimation.getElapsedTime(), NULL);
	}
	else
	{
		heightAnimation = DoubleAnimation(startHeight,targetHeight,duration, NULL);
	}


	if (duration > 0)
	{
		if (width != targetWidth)
			widthAnimation.start();

		if (height != targetHeight)
			heightAnimation.start();
	}	

	
	width = targetWidth;
	height = targetHeight;
	this->lastSize = cv::Size2f(targetWidth,targetHeight);
}

void PanelBase::layout(cv::Size2f size)
{
	;
}



void PanelBase::drawLoadTimer(Vector drawPosition, float drawWidth, float drawHeight)
{
	
	glBindTexture( GL_TEXTURE_2D, NULL);		
	
	Color loadColor = loadAnimationColor;
	glColor4fv(loadColor.getFloat());
	//float lineWidth = 1.0f;
	for (int i = 0;i<5;i++)
	{		
		float angle = loadAnimTimer.seconds()/4.0 + ((float)(i*i)/10);

		float x1,x2,x3,x4;
		float angle2 = fmod(angle,2);


		x2 = x1 = (angle2*.5f) * drawWidth;

		float lineWidth =  10.0f * (.5f + sin(angle*GeomConstants::PI_F));

		x4 = x3 = min<float>(x1 + lineWidth,drawWidth);

		float y1,y2,y3,y4;

		y4 = y1 = 0;
		y3 = y2 = drawHeight;

		float z1 = 0;

		glTranslatef(drawPosition.x,drawPosition.y,drawPosition.z+.5f);

		glBegin( GL_QUADS );

		glVertex3f(x1,y1,z1);
		glVertex3f(x2,y2,z1);
		glVertex3f(x3,y3,z1);
		glVertex3f(x4,y4,z1);

		glEnd();

		glTranslatef(-drawPosition.x,-drawPosition.y,-(drawPosition.z+.5f));
	}



	//Color loadColor = loadAnimationColor;
	////float lineWidth = 1.0f;

	//glTranslatef(drawPosition.x,drawPosition.y,drawPosition.z+.5f);
	//glBegin(GL_QUAD_STRIP);

	//for (int i = 0;i<5;i++)
	//{		
	//	float angle = loadAnimTimer.seconds()/4.0 + ((float)(i*i)/10);
	//	
	//	float y1 = 0,y2 = drawHeight;
	//	float x1,x2,x3,x4;
	//	float z1 = 0;
	//	float angle2 = fmod(angle,2);

	//	float gradWidth = 10.0f;
	//	float lineWidth = 4.0f; // 10.0f * (.5f + sin(angle*GeomConstants::PI_F));

	//	x1 = (angle2*.5f) * drawWidth;
	//	x2 = min<float>(x1 + gradWidth,drawWidth);
	//	float x25 = min<float>(x2 + lineWidth*.5f,drawWidth);
	//	x3 = min<float>(x2 + lineWidth,drawWidth);
	//	x4 = min<float>(x3 + gradWidth,drawWidth);
	//	
	//	
	//	glColor4fv(loadColor.withAlpha(.2f).getFloat());
	//	glVertex3f(x1,y1,z1);
	//	glVertex3f(x1,y2,z1);		
	//	
	//	glColor4fv(loadColor.withAlpha(.6f).getFloat());
	//	glVertex3f(x2,y1,z1);
	//	glVertex3f(x2,y2,z1);	
	//	
	//	glColor4fv(loadColor.withAlpha(.9f).getFloat());
	//	glVertex3f(x25,y1,z1);
	//	glVertex3f(x25,y2,z1);	

	//	glVertex3f(x3,y1,z1);
	//	glVertex3f(x3,y2,z1);
	//	
	//	glColor4fv(loadColor.withAlpha(.2f).getFloat());
	//	glVertex3f(x4,y1,z1);
	//	glVertex3f(x4,y2,z1);		

	//}

	//glEnd();
	//glTranslatef(-drawPosition.x,-drawPosition.y,-(drawPosition.z+.5f));

}


void PanelBase::setDrawLoadAnimation(bool drawLoadAnim)
{
	loadAnimTimer.start();
	drawingLoadAnimation = drawLoadAnim;
}

void PanelBase::getBoundingArea(Vector & drawPosition, float & drawWidth, float & drawHeight)
{	
	drawWidth = width;
	drawHeight = height;
	
	if (widthAnimation.isRunning())
	{
		float _drawWidth = widthAnimation.getValue();
		panelDirty = panelDirty || (drawWidth != _drawWidth);
		drawWidth = _drawWidth;
	}
	if (heightAnimation.isRunning())
	{
		float _drawHeight = heightAnimation.getValue();		
		panelDirty = panelDirty || (drawHeight != _drawHeight);
		drawHeight = _drawHeight;
	}

	if (scaleXAnimation.isRunning())
	{	
		float _drawWidth = drawWidth * scaleXAnimation.getValue();
		panelDirty = panelDirty ||  (drawWidth != _drawWidth);
		drawWidth = _drawWidth;
	}
	if (scaleYAnimation.isRunning())
	{
		float _drawHeight = drawHeight * scaleYAnimation.getValue();
		panelDirty = panelDirty || (drawHeight != _drawHeight);
		drawHeight = _drawHeight;
	}
			
	drawPosition = Vector(position);

	if (xPosAnimation.isRunning())
	{
		float _x = xPosAnimation.getValue();	
		panelDirty = panelDirty || (_x != drawPosition.x);
		drawPosition.x  = _x;
	}

	if (yPosAnimation.isRunning())	
	{
		float _y = yPosAnimation.getValue();	
		panelDirty = panelDirty || (_y != drawPosition.y);
		drawPosition.y  = _y;
	}

	if (zPosAnimation.isRunning())	
	{
		float _z = zPosAnimation.getValue();	
		panelDirty = panelDirty || (_z != drawPosition.z);
		drawPosition.z  = _z;
	}
	
	
	if (xAnimation.isRunning())
		drawPosition.x += xAnimation.getValue();
	if (yAnimation.isRunning())
		drawPosition.y += yAnimation.getValue();	
	if (zAnimation.isRunning())
		drawPosition.z += zAnimation.getValue();
	
	
	if (panelDirty)
	{
		layout(cv::Size2f(drawWidth,drawHeight));
	}
}

void PanelBase::setPosition(Vector position)
{	
	this->lastPosition = position;
	this->position = position;
	panelDirty = true;
}

void PanelBase::abortAnimations()
{
	xPosAnimation.stop();
	yPosAnimation.stop();
	zPosAnimation.stop();
	widthAnimation.stop();
	heightAnimation.stop();
	panelDirty = true;
}


void PanelBase::draw()
{ 
	float drawWidth,drawHeight;
	Vector drawPosition;		
	getBoundingArea(drawPosition,drawWidth,drawHeight);
	drawPanel(drawPosition, drawWidth, drawHeight);
}

cv::Rect_<int> PanelBase::getHitRect()
{
	float drawWidth,drawHeight;
	Vector drawPosition;

	cv::Rect_<int> boundingRect;

	getBoundingArea(drawPosition,drawWidth,drawHeight);	

	boundingRect = cv::Rect_<int>((int)drawPosition.x,(int)drawPosition.y,(int)drawWidth,(int)drawHeight);

	return boundingRect;
}

void PanelBase::OnPointableExit(Pointable & pointable)
{
	if (BorderSelectionThickness > 0)
	{
		this->borderThickness = beforeSelectionBorderThickness;
	}	

	if (ScaleOnEnter != 1.0f)
	{
		if (scaleXAnimation.isRunning() || scaleYAnimation.isRunning())
		{
			scaleXAnimation = DoubleAnimation(ScaleOnEnter,1,100);
			scaleYAnimation = DoubleAnimation(ScaleOnEnter,1,100);

			scaleXAnimation.start();
			scaleYAnimation.start();
		}
		//else
		//{			
		//	scaleXAnimation = DoubleAnimation(scaleXAnimation.getValue(),1,300);
		//	scaleYAnimation = DoubleAnimation(scaleYAnimation.getValue(),1,300);

		//	scaleXAnimation.start();
		//	scaleYAnimation.start();
		//}
	}

}

void PanelBase::OnPointableEnter(Pointable & pointable)
{	
	if (NudgeAnimationEnabled)
	{
		if (!xAnimation.isRunning() && !yAnimation.isRunning())
		{
			if (pointable.tipVelocity().magnitude() > 10)
			{		
				float xNudge = pointable.tipVelocity().x / 10.0f;
				xNudge = min<float>(30,xNudge);
				xNudge = max<float>(-30,xNudge);

				float yNudge = pointable.tipVelocity().y / 10.0f;
				yNudge = min<float>(30,yNudge);
				yNudge = max<float>(-30,yNudge);		

				xAnimation = DoubleAnimation(0, xNudge,300,NULL,true);
				yAnimation = DoubleAnimation(0,-yNudge,300, NULL, true);

				xAnimation.start();
				yAnimation.start();
			}
		}
	}

	if (BorderSelectionThickness > 0 && this->borderThickness != BorderSelectionThickness)
	{
		beforeSelectionBorderThickness = this->borderThickness;
		this->borderThickness = BorderSelectionThickness;
	}

	if (ScaleOnEnter != 1.0f)
	{
		if (!scaleXAnimation.isRunning() && !scaleYAnimation.isRunning())
		{
			scaleXAnimation = DoubleAnimation(1,ScaleOnEnter,300,NULL,false,true);
			scaleYAnimation = DoubleAnimation(1,ScaleOnEnter,300,NULL,false,true);

			scaleXAnimation.start();
			scaleYAnimation.start();
		}
	}
}


void PanelBase::drawPanel(Vector drawPosition, float drawWidth, float drawHeight)
{		
	drawBackground(drawPosition,drawWidth,drawHeight);
	if (drawingLoadAnimation)
	{
		drawLoadTimer(drawPosition,drawWidth,drawHeight);
	}
	drawContent(drawPosition+Vector(0,0,1),drawWidth,drawHeight);	
}

void PanelBase::drawBackground(Vector drawPosition, float drawWidth, float drawHeight)
{	
	float x1 = drawPosition.x; //- drawWidth/2.0f;
	float x2 = x1 + drawWidth;
	float y1 = drawPosition.y ;//- drawHeight/2.0f;
	float y2 = y1 + drawHeight;
	float z1 = drawPosition.z;
	
	glBindTexture( GL_TEXTURE_2D, NULL);

	if (backgroundColor.colorArray[3] > 0.0f)
	{
		glColor4fv(backgroundColor.getFloat());

		glLineWidth(0);		

		glBegin( GL_QUADS );
		glVertex3f(x1,y1,z1);
		glVertex3f(x2,y1,z1);
		glVertex3f(x2,y2,z1);
		glVertex3f(x1,y2,z1);
		glEnd();
	}

	if (borderThickness > 0)
	{
		if (!useLineBorder)
		{
			glColor4fv(borderColor.getFloat());

			float b = borderThickness;

			x1 += b;
			y1 += b;
			x2 -= b;
			y2 -= b;
			z1 += 0.5f;

			//Left edge
			glBegin( GL_QUADS );

			glVertex3f(x1-b,y1,z1);
			glVertex3f(x1,y1,z1);
			glVertex3f(x1,y2,z1);
			glVertex3f(x1-b,y2,z1);

			//Right Edge
			glVertex3f(x2,y1,z1);
			glVertex3f(x2+b,y1,z1);
			glVertex3f(x2+b,y2,z1);
			glVertex3f(x2,y2,z1);

			//Top Edge
			glVertex3f(x1-b,y1-b,z1);
			glVertex3f(x1-b,y1,z1);
			glVertex3f(x2+b,y1,z1);
			glVertex3f(x2+b,y1-b,z1);

			//Bottom Edge
			glVertex3f(x1-b,y2,z1);
			glVertex3f(x1-b,y2+b,z1);
			glVertex3f(x2+b,y2+b,z1);
			glVertex3f(x2+b,y2,z1);

			glEnd();


			//float b = borderThickness *.5f;
			////Left edge
			//glBegin( GL_QUADS );

			//	glVertex3f(x1-b,y1,z1);
			//	glVertex3f(x1+b,y1,z1);
			//	glVertex3f(x1+b,y2,z1);
			//	glVertex3f(x1-b,y2,z1);

			////Right Edge
			//	glVertex3f(x2-b,y1,z1);
			//	glVertex3f(x2+b,y1,z1);
			//	glVertex3f(x2+b,y2,z1);
			//	glVertex3f(x2-b,y2,z1);
			//		
			////Top Edge
			//	glVertex3f(x1-b,y1-b,z1);
			//	glVertex3f(x1-b,y1+b,z1);
			//	glVertex3f(x2+b,y1+b,z1);
			//	glVertex3f(x2+b,y1-b,z1);

			////Bottom Edge
			//	glVertex3f(x1-b,y2-b,z1);
			//	glVertex3f(x1-b,y2+b,z1);
			//	glVertex3f(x2+b,y2+b,z1);
			//	glVertex3f(x2+b,y2-b,z1);

			//glEnd();
		}
		else
		{
			glLineWidth(borderThickness);	
			glColor4fv(borderColor.getFloat());
			glBegin( GL_LINE_LOOP);
			glVertex3f(x1,y1,z1);
			glVertex3f(x2,y1,z1);
			glVertex3f(x2,y2,z1);
			glVertex3f(x1,y2,z1);
			glEnd();
		}
	}

}


void PanelBase::animateToPosition(Vector newPosition, long durationX, long durationY)
{
	if (position.distanceTo(newPosition) == 0.0f)
		return;

	Vector startPosition = position;

	if (xPosAnimation.isRunning())
	{
		startPosition.x = xPosAnimation.getValue();
		durationX -= xPosAnimation.getElapsedTime();
	}
	if (yPosAnimation.isRunning())
	{
		startPosition.y = yPosAnimation.getValue();
		durationY -= yPosAnimation.getElapsedTime();
	}
	if (zPosAnimation.isRunning())
		startPosition.z = zPosAnimation.getValue();

	if (durationX < 0)
	{
		durationX = newPosition.distanceTo(position);
		durationX = min<long>(3000,durationX);
		durationX = max<long>(10,durationX);
	}
	
	if (durationY < 0)
	{
		durationY = newPosition.distanceTo(position);
		durationY = min<long>(3000,durationY);
		durationY  = max<long>(10,durationY);
	}
		
	xPosAnimation = DoubleAnimation(startPosition.x,newPosition.x,durationX,NULL);
	xPosAnimation.start();
	
	yPosAnimation = DoubleAnimation(startPosition.y,newPosition.y,durationY,NULL);
	yPosAnimation.start();

	zPosAnimation = DoubleAnimation(startPosition.z,newPosition.z,max(durationX,durationY),NULL);
	zPosAnimation.start();
	
	panelDirty = true;
	position = newPosition;
}

void PanelBase::update(double delta)
{

}

float PanelBase::getZValue()
{
	return this->position.z;
}