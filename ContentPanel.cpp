#include "ContentPanel.hpp"


ContentPanel::ContentPanel()
{
	this->eatEvents = false;
	this->contentView = NULL;
}

ContentPanel::ContentPanel(View * _contentView)
{
	this->eatEvents = false;
	this->contentView = _contentView;
}

void ContentPanel::drawContent(Vector drawPosition, float drawWidth, float drawHeight)
{
	this->setBorderColor(Colors::Transparent);
	this->setBorderThickness(0);
	if (contentView != NULL && contentView->isVisible())
	{
		if (NudgeAnimationEnabled)
		{
			float yNudge =(yAnimation.isRunning()) ? yAnimation.getValue() : 0;
			float xNudge =(xAnimation.isRunning()) ? xAnimation.getValue() : 0;
			
			xNudge = floor(xNudge);
			yNudge = floor(yNudge);

			glTranslatef(xNudge,yNudge,0);
			contentView->draw();
			glTranslatef(-xNudge,-yNudge,0);
		}
		else
		{
			contentView->draw();
		}

	}
}

void ContentPanel::setContentView(View * _contentView)
{
	this->contentView = _contentView;
}

View * ContentPanel::getContentView()
{
	return this->contentView;
}

void ContentPanel::measure(cv::Size2f & measuredSize)
{
	this->contentView->measure(measuredSize);
}

void ContentPanel::layout(Vector layoutPosition, cv::Size2f layoutSize)
{
	PanelBase::layout(layoutPosition,layoutSize);

	lastSize = layoutSize;
	lastPosition = layoutPosition;

	if (this->contentView != NULL && contentView->isVisible())
		contentView->layout(layoutPosition,layoutSize);
}

LeapElement * ContentPanel::elementAtPoint(int x, int y, int & elementStateFlags)
{
	if (eatEvents) //nom
		return LeapElement::elementAtPoint(x,y,elementStateFlags);
	else
	{
		if (contentView != NULL)
			return contentView->elementAtPoint(x,y,elementStateFlags);
		else
			return NULL;
	}
}

void ContentPanel::setEventHandlingMode(bool _eatEvents)
{
	this->eatEvents = _eatEvents;
}

bool ContentPanel::getEventHandlingMode()
{
	return eatEvents;
}