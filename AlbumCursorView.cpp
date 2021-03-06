﻿#include "AlbumCursorView.hpp"
#include "ViewOrchestrator.hpp"

using namespace Facebook;

AlbumCursorView::AlbumCursorView() : DataListActivity(2)
{	
	imageDetailView = new PictureDetailView();
	imageDetailView->setVisible(false);
	imageDetailView->setFinishedCallback([this](string a){
		LeapInput::getInstance()->releaseGlobalGestureFocus(this->imageDetailView);	
		this->imageDetailView->setVisible(false);
		this->layoutDirty = true;						
	});

	addChild(imageDetailView);
}

void AlbumCursorView::setAlbumOwner(FBNode * _albumOwner)
{
	this->albumOwner = _albumOwner;

	FBAlbumPhotosCursor * photosCursor = new FBAlbumPhotosCursor(albumOwner);
	photosCursor->getNext();

	this->show(photosCursor);
	
	TextPanel * albumHeading = dynamic_cast<TextPanel*>(ViewOrchestrator::getInstance()->requestView(albumOwner->getId() + "/name", this));

	if (albumHeading == NULL)
	{
		albumHeading = new TextPanel(albumOwner->getAttribute("name"));
		ViewOrchestrator::getInstance()->registerView(albumOwner->getId() + "/name",albumHeading,this);
	}
	albumHeading->setStyle(GlobalConfig::tree()->get_child("AlbumDetailView.Title"));

	setTitlePanel(albumHeading);
}

FBNode * AlbumCursorView::getAlbumOwner()
{
	return this->albumOwner;
}

void AlbumCursorView::refreshDataView(DataNode * node, View * view)
{
	;
}
	


void AlbumCursorView::setItemPriority(float priority, View * itemView)
{
	PicturePanel * picture = dynamic_cast<PicturePanel*>(itemView);
	if (picture != NULL)
		picture->setDataPriority(priority);
}

void AlbumCursorView::childPanelClicked(FBNode * childNode)
{
	itemScroll->getFlyWheel()->impartVelocity(0);			
	imageDetailView->notifyOffsetChanged(Vector((float)this->itemScroll->getFlyWheel()->getCurrentPosition(),0,0));
	
	FBAlbumPhotosCursor * f1 = new FBAlbumPhotosCursor(albumOwner);
	f1->getNext();
	WrappingBDCursor * wrapCursor = new WrappingBDCursor(f1);
	wrapCursor->fastForward(childNode);

	FBAlbumPhotosCursor * f2 = new FBAlbumPhotosCursor(albumOwner);
	f2->getNext();
	WrappingBDCursor * revWrapCursor = new WrappingBDCursor(f2);

	revWrapCursor->fastForward(childNode);

	imageDetailView->setCursor(revWrapCursor,wrapCursor);										
	imageDetailView->setVisible(true);
	LeapInput::getInstance()->requestGlobalGestureFocus(this->imageDetailView);

	layoutDirty = true;		
}

View * AlbumCursorView::getDataView(DataNode * dataNode)
{
	FBNode * node = (FBNode*)dataNode;

	View * v= ViewOrchestrator::getInstance()->requestView(node->getId(), this);

	PicturePanel * item = NULL;
	if (v == NULL)
	{
		item = new PicturePanel();
		item->show(node);
		ViewOrchestrator::getInstance()->registerView(node->getId(),item, this);
	}
	else
	{
		item = dynamic_cast<PicturePanel*>(v);
	}

	item->setLayoutParams(LayoutParams(cv::Size2f(),cv::Vec4f(5,5,5,5)));
	item->layout(Vector(lastSize.width-itemScroll->getFlyWheel()->getPosition(),lastSize.height*.5f,-10),cv::Size2f(lastSize.height*(1.0f/((float)rowCount)),10));
	item->setClickable(true);
	item->setVisible(true);

	item->elementClickedCallback = [this,node](LeapElement * clicked){
		this->childPanelClicked(node);	
	};

	return item;
}


void AlbumCursorView::showPhoto(FBNode * photoNode)
{
	View * dataView = getDataView(photoNode);

	PicturePanel * pp =dynamic_cast<PicturePanel*>(dataView);
	if (pp != NULL)
	{
		pp->elementClicked();
	}
}

void AlbumCursorView::getTutorialDescriptor(vector<string> & tutorial)
{
	tutorial.push_back("swipe");
	tutorial.push_back("point_stop");
	tutorial.push_back("shake");
}

void AlbumCursorView::setFinishedCallback(const boost::function<void(std::string)> & callback)
{
	viewFinishedCallback = callback;
}

void AlbumCursorView::viewOwnershipChanged(View * view, ViewOwner * newOwner)
{	
	if (newOwner != imageDetailView)
	{
		itemGroup->remove(view);
		view->elementClickedCallback = [](LeapElement * e){};
	}
}


void AlbumCursorView::onGlobalGesture(const Controller & controller, std::string gestureType)
{
	if (gestureType.compare("shake") == 0)
	{
		viewFinishedCallback("done");
	} 
	else if (gestureType.compare("pointing") == 0)
	{
		itemScroll->getFlyWheel()->impartVelocity(0);
	}
}

void AlbumCursorView::layout(Vector position, cv::Size2f size)
{
	DataListActivity::layout(position,size);
	
	if (imageDetailView->isVisible())
	{
		imageDetailView->layout(position,size);
	}
}