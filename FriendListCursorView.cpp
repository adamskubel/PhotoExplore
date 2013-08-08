#include "FriendListCursorView.hpp"
#include "CustomGrid.hpp"
#include "Button.hpp"

FriendListCursorView::FriendListCursorView() : DataListActivity(3)
{
	showPicturelessFriends = false;

	auto editTextConfig = GlobalConfig::tree()->get_child("FriendLookupView.LookupDialog.FriendNameTextBox");

	editText = new TextEditPanel();
	editText->setTextSize(editTextConfig.get<float>("TextSize"),true);
	editText->setTextColor(Color(editTextConfig.get_child("TextColor")));
	editText->setBackgroundColor(Color(editTextConfig.get_child("BackgroundColor")));
	editText->setBorderColor(Color(editTextConfig.get_child("BorderColor")));
	editText->setBorderThickness(editTextConfig.get<float>("BorderThickness"));
	editText->setTextFitPadding(editTextConfig.get<float>("TextPadding"));
	editText->setTextAlignment(editTextConfig.get<int>("TextAlignment"));
	editText->setMaxLength(editTextConfig.get<int>("MaxLength"));

	

	vector<RowDefinition> gridDefinition;	
	gridDefinition.push_back(RowDefinition(.35f));
	gridDefinition.push_back(RowDefinition(.65f));
	gridDefinition[0].ColumnWidths.push_back(1);
	gridDefinition[1].ColumnWidths.push_back(1);
	
	editTextConfig = GlobalConfig::tree()->get_child("FriendLookupView.LookupDialog.FriendLookupLabel");

	labelText = new TextPanel(editTextConfig.get<string>("Text"));
	labelText->setTextSize(editTextConfig.get<float>("TextSize"),true);
	labelText->setTextColor(Color(editTextConfig.get_child("TextColor")));
	labelText->setBackgroundColor(Color(editTextConfig.get_child("BackgroundColor")));
	labelText->setBorderColor(Color(editTextConfig.get_child("BorderColor")));
	labelText->setBorderThickness(editTextConfig.get<float>("BorderThickness"));
	labelText->setTextFitPadding(editTextConfig.get<float>("TextPadding"));
	labelText->setTextAlignment(editTextConfig.get<int>("TextAlignment"));

	CustomGrid * lookupDialogGrid = new CustomGrid(gridDefinition);
	lookupDialogGrid->addChild(labelText);	
	lookupDialogGrid->addChild(editText);
	
	lookupPanel = lookupDialogGrid;
	
	addChild(lookupDialogGrid);

	lookupDialogState = 0;

	int dialogHideDelay = GlobalConfig::tree()->get<int>("FriendLookupView.LookupDialog.HideAfter");
	int dialogHideDelayMovement = GlobalConfig::tree()->get<int>("FriendLookupView.LookupDialog.HideAfterWithMovement");

	editText->setTextChangedCallback([this,dialogHideDelay,dialogHideDelayMovement](string newText){
		
		lookupDialogTimer.countdown(dialogHideDelay);
		lookupDialogMovementTimer.countdown(dialogHideDelayMovement);

		if (lookupDialogState != 2 && newText.length() > 0)
		{
			lookupDialogState = 2;
			layoutDirty = true;			
		}		
		
		if (lookupDialogState != 0 && newText.length() == 0)
		{			
			lookupDialogState = 0;
			layoutDirty = true;			
			
			showPicturelessFriends = false;
			((FBFriendsCursor*)allFriendsCursor)->reset();			
			this->show(allFriendsCursor);
			allFriendsCursor->getNext();
		}
		else		
		{
			if (this->cursor != searchCursor)
			{
				searchCursor->lookupName(newText);				
				this->show(searchCursor);			
				searchCursor->getNext();
				//showPicturelessFriends = true;
			}
			else
			{
				currentRightBoundary = 0;
				lastUpdatePos = 100000;
				itemScroll->getFlyWheel()->overrideValue(0);

				pendingItems.clear();
				items.clear();
				itemGroup->clearChildren();

				searchCursor->lookupName(newText);		
				searchCursor->getNext();
				this->updateLoading();
			}
		}
	});

}

void FriendListCursorView::suspend()
{
	DataListActivity::suspend();
}

void FriendListCursorView::resume()
{
	DataListActivity::resume();
	this->layoutDirty;
}

void FriendListCursorView::setUserNode(FBNode * node)
{	
	editText->setText("");
	lookupDialogState = 0;
	layoutDirty = true;

	showPicturelessFriends = false;

	searchCursor = new FBFriendsFQLCursor("",node);
	allFriendsCursor = new FBFriendsCursor(node);

	allFriendsCursor->getNext();
	this->show(allFriendsCursor);
}

FBDataView * FriendListCursorView::getDataView(FBNode * itemNode)
{
	FBDataView * itemV = NULL;
	bool hasData =(itemNode->Edges.get<EdgeTypeIndex>().count("photos") + itemNode->Edges.get<EdgeTypeIndex>().count("albums") >= 2);
	if (showPicturelessFriends || hasData)
	{
		FriendPanel * item = (FriendPanel*)ViewOrchestrator::getInstance()->requestView(itemNode->getId(), this);

		if (item == NULL)
		{
			item = new FriendPanel(cv::Size2f(600,400));					
			ViewOrchestrator::getInstance()->registerView(itemNode->getId(),item, this);
		}

		if (hasData)
		{
			item->elementClickedCallback = [item](LeapElement*element){
				FacebookDataDisplay::getInstance()->displayNode(item->getNode(),"");
			};
		}
		item->setLayoutParams(LayoutParams(cv::Size2f(600,400),cv::Vec4f(5,5,5,5)));
		item->setVisible(true);
		item->show(itemNode);
		itemV = item;
	}
	else if (itemNode->getAttribute("tried_load").length() == 0)
	{			
		FBDataView * nullView = NULL;
		pendingItems.insert(itemNode);
		itemNode->Edges.insert(Edge("tried_load","1"));
		stringstream load2;
		load2 << itemNode->getId() << "?fields=photos.fields(id,name,images,album).limit(2),albums.fields(name,updated_time,photos.fields(id,name,images,album).limit(4)).limit(2)";		
		FBDataSource::instance->loadField(itemNode,load2.str(),"",[itemNode,this](FBNode * nn)
		{
			FriendListCursorView * me = this;
			this->postTask([me,nn](){
				if (me->pendingItems.count(nn) > 0)
				{
					me->pendingItems.erase (nn);
					me->addNode(nn);
				}
			});
		}); 
	}
	return itemV;
}

void FriendListCursorView::setFinishedCallback(const boost::function<void(std::string)> & callback)
{
	viewFinishedCallback = callback;
}

void FriendListCursorView::onGlobalGesture(const Controller & controller, std::string gestureType)
{
	if (gestureType.compare("shake") == 0)
	{
		if (lookupDialogState != 0 && this->cursor == searchCursor)
		{
			lookupDialogState = 0;
			layoutDirty = true;
			
			((FBFriendsCursor*)allFriendsCursor)->reset();
			this->show(allFriendsCursor);
			allFriendsCursor->getNext();
			this->editText->setText("");
		}
		else
		{
			viewFinishedCallback("done");
		}
	} 
	else if (gestureType.compare("pointing") == 0)
	{
		itemScroll->getFlyWheel()->impartVelocity(0);
	}
}

bool FriendListCursorView::onLeapGesture(const Controller & controller, const Gesture & gesture)
{
	return itemScroll->onLeapGesture(controller, gesture);
}

void FriendListCursorView::getTutorialDescriptor(vector<string> & tutorial)
{	
	tutorial.push_back("swipe");
	tutorial.push_back("point_stop");
	tutorial.push_back("shake");
	tutorial.push_back("KeyboardNameSearch");
}

void FriendListCursorView::layout(Vector position, cv::Size2f size)
{
	Logger::stream("FriendListCursorView","INFO") << "Layout. Position = " << position.toString() << endl;
	DataListActivity::layout(position,size);

	float menuHeight = GlobalConfig::tree()->get<float>("Menu.Height");
	
	static long hideTime = GlobalConfig::tree()->get<long>("FriendLookupView.LookupDialog.HideTime");
	static long showTime = GlobalConfig::tree()->get<long>("FriendLookupView.LookupDialog.ShowTime");

	if (lookupDialogState == 2)
	{
		editText->setAnimateOnLayout(showTime > 0);
		labelText->setAnimateOnLayout(showTime > 0);
		editText->setLayoutDuration(showTime);
		labelText->setLayoutDuration(showTime);
	}
	else
	{
		editText->setLayoutDuration(hideTime);
		editText->setAnimateOnLayout(hideTime > 0);

		labelText->setLayoutDuration(hideTime);
		labelText->setAnimateOnLayout(hideTime > 0);
	}
	
	cv::Size2f dialogSize = cv::Size2f(GlobalConfig::tree()->get<float>("FriendLookupView.LookupDialog.Width"),GlobalConfig::tree()->get<float>("FriendLookupView.LookupDialog.Height"));
	dialogSize.width *= size.width;
	dialogSize.height *= menuHeight;

	Vector dialogPosition = position + Vector((size.width - dialogSize.width)*.5f,-menuHeight + (menuHeight - dialogSize.height)*.5f,10);

	if (lookupDialogState == 0)
	{
		dialogPosition.y -= menuHeight;
	}
	editText->layout(dialogPosition+Vector(dialogSize.width*.25f,0,0),cv::Size2f(dialogSize.width*.75,dialogSize.height));
	labelText->layout(dialogPosition-Vector(dialogSize.width*.25f,0,0),cv::Size2f(dialogSize.width*.5f,dialogSize.height));
}


void FriendListCursorView::update()
{
	if (lookupDialogState == 2)
	{
		if (lookupDialogTimer.elapsed())
		{
			lookupDialogState = 1;
			layoutDirty = true;			
		}
		else if (lookupDialogMovementTimer.elapsed())
		{
			double pos = itemScroll->getFlyWheel()->getPosition();
			if (abs(lastUpdatePos - (-pos)) > 100)
			{
				lookupDialogState = 1;
				layoutDirty = true;		
			}
		}
	}
	
	DataListActivity::update();
}