#include "RadialMenu.hpp"
#include "GraphicContext.hpp"
#include "GLImport.h"
#include "Button.hpp"
#include "FixedAspectGrid.hpp"
#include "UniformGrid.hpp"
#include "CustomGrid.hpp"
#include "LeapDebug.h"
#include "ImageButton.hpp"

RadialMenu * RadialMenu::instance = NULL;

RadialMenu::RadialMenu(vector<RadialMenuItem> & items)
{			
	items.push_back(RadialMenuItem("Exit and Logout","logout", Colors::DarkRed));
	items.push_back(RadialMenuItem("Exit PhotoExplore","exit",Colors::OrangeRed));

	if (GlobalConfig::tree()->get<bool>("Menu.EnableFullscreenToggle"))
		items.push_back(RadialMenuItem("Toggle Fullscreen","full",Colors::DarkSlateBlue));

	//items.push_back(RadialMenuItem("Hide Tutorial","hide_tutorial", Colors::DarkTurquoise));
	//items.push_back(RadialMenuItem("Tap mode = On Press","toggle_tap", Colors::DarkTurquoise));
	items.push_back(RadialMenuItem("Privacy Information","show_privacy_info", Colors::DodgerBlue));
	items.push_back(RadialMenuItem("Back","cancel",Colors::SkyBlue));

	menuLaunchButton =  new Button("Menu"); 
	((Button*)menuLaunchButton)->setTextSize(10,false);
	((Button*)menuLaunchButton)->setTextColor(Colors::SteelBlue);
	((Button*)menuLaunchButton)->setTextAlignment(1);
	((Button*)menuLaunchButton)->setTextFitMode(true);
	((Button*)menuLaunchButton)->setTextFitPadding(GlobalConfig::tree()->get<float>("Menu.CornerPadding"));
	addChild(menuLaunchButton);
	menuLaunchButton->elementClickedCallback = [this](LeapElement * clicked){

		if (this->state == MenuState_ButtonOnly)
		{
			this->show();
		}
	};

	TextPanel * privacyInfoText = new TextPanel(GlobalConfig::tree()->get<string>("Menu.PrivacyInfo.Text"));
	privacyInfoText->setTextSize(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.TextSize"),false);
	privacyInfoText->setTextColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.TextColor")));
	privacyInfoText->setBackgroundColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.BackgroundColor")));
	privacyInfoText->setBorderColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.BorderColor")));
	privacyInfoText->setBorderThickness(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.BorderThickness"));
	privacyInfoText->setTextFitPadding(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.TextPadding"));
	privacyInfoText->setTextAlignment(GlobalConfig::tree()->get<int>("Menu.PrivacyInfo.TextAlignment"));
	privacyInfoText->reloadText();


	Button * dismissDialogButton = new Button(GlobalConfig::tree()->get<string>("Menu.PrivacyInfo.DismissButton.Text"));
	dismissDialogButton->setTextSize(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DismissButton.TextSize"),false);
	dismissDialogButton->setTextColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.DismissButton.TextColor")));
	dismissDialogButton->setBackgroundColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.DismissButton.BackgroundColor")));
	dismissDialogButton->setBorderColor(Color(GlobalConfig::tree()->get_child("Menu.PrivacyInfo.DismissButton.BorderColor")));
	dismissDialogButton->setBorderThickness(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DismissButton.BorderThickness"));
	dismissDialogButton->elementClickedCallback = [this](LeapElement * clicked){

		if (this->state == MenuState_ShowingDialog)
		{
			this->state = MenuState_DisplayFull;
			this->layout(lastPosition,lastSize);
		}

	};

	
	vector<RowDefinition> gridDefinition;	
	gridDefinition.push_back(RowDefinition(.8f));
	gridDefinition.push_back(RowDefinition(.2f));
	gridDefinition[0].ColumnWidths.push_back(1);
	gridDefinition[1].ColumnWidths.push_back(1);

	CustomGrid * privacyInfoGrid = new CustomGrid(gridDefinition);
	((ViewGroup*)privacyInfoGrid)->addChild(privacyInfoText);	
	((ViewGroup*)privacyInfoGrid)->addChild(dismissDialogButton);
	
	privacyInfoBox = privacyInfoGrid;
	
	cv::Size2f dialogSize = cv::Size2f(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogWidth"),GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogHeight"));
	privacyInfoBox->setVisible(false);


	state = MenuState_ButtonOnly;
	instance = this;
	
	setItems(items);
	
	GraphicsContext::getInstance().requestConstantClarity(menuLaunchButton);
}

void RadialMenu::setItems(vector<RadialMenuItem> & items)
{
	clearChildren();		
	for (auto it = items.begin(); it != items.end(); it++)
	{
		Button * item = new Button(it->label);
		item->setBorderColor(it->buttonColor.withAlpha(1.0f));
		item->setBorderThickness(2);
		item->setBackgroundColor(it->buttonColor.withAlpha(.7f));		
		
		if (GlobalConfig::tree()->get<bool>("Menu.WhiteBackground"))
			item->setBackgroundColor(Colors::White);

		item->setTextColor(GlobalConfig::tree()->get_child("Menu.TextColor"));
		item->setTextSize(12);
		string itemId = it->id;
		item->setLayoutParams(LayoutParams(cv::Size2f(400,150),cv::Vec4f(20,20,20,20)));
		item->elementClickedCallback = [this,itemId,item](LeapElement * element){
			this->itemClicked(itemId,item);
		};
		addChild(item);
	}	
	addChild(privacyInfoBox);
	addChild(menuLaunchButton);
	layoutDirty = true;
}


LeapElement * RadialMenu::elementAtPoint(int x, int y, int & _state)
{
	if (state == MenuState_DisplayFull)
	{
		LeapElement * hit = ViewGroup::elementAtPoint(x,y,_state);

		if (hit == NULL)
			return this;
		else
			return hit;
	}
	else if (state == MenuState_ShowingDialog)
	{
		return privacyInfoBox->elementAtPoint(x,y,state);
	}
	else
	{
		return menuLaunchButton->elementAtPoint(x,y,_state);
	}
}

void RadialMenu::OnElementClicked(Pointable & pointable)
{
	this->dismiss();
}


float RadialMenu::getZValue()
{
	return 100.0f;
}

void RadialMenu::itemClicked(string id, Button * menuButton)
{
	if (id.compare("cancel") == 0)
	{
		this->dismiss();
	}
	else if (id.compare("show_privacy_info") == 0)
	{
		state = MenuState_ShowingDialog;
		privacyInfoBox->setVisible(true);
		layout(lastPosition,lastSize);
	}	
	else if (id.compare("toggle_tap") == 0)
	{
		if (GlobalConfig::tree()->get<bool>("Leap.TouchDistance.ClickOnRelease"))
		{
			menuButton->setText("Tap mode = On Press");
			GlobalConfig::tree()->put<bool>("Leap.TouchDistance.ClickOnRelease",false);
			menuButton->reloadText();
		}
		else
		{
			menuButton->setText("Tap mode = On Release");
			GlobalConfig::tree()->put<bool>("Leap.TouchDistance.ClickOnRelease",true);
			menuButton->reloadText();
		}
	}
	//else if (id.compare("hide_tutorial") == 0)
	//{
	//	GlobalConfig::tree()->put<bool>("Tutorial.Enabled",false);
	//	vector<string> x;
	//	LeapDebug::getInstance().setTutorialImages(x);

	//	vector<RadialMenuItem> items;
	//	items.push_back(RadialMenuItem("Exit and Logout","logout", Colors::DarkRed));
	//	items.push_back(RadialMenuItem("Exit Photo Explorer","exit",Colors::OrangeRed));
	//	items.push_back(RadialMenuItem("Show Tutorial","show_tutorial", Colors::DarkTurquoise));
	//	items.push_back(RadialMenuItem("Privacy Information","show_privacy_info", Colors::DeepSkyBlue));
	//	items.push_back(RadialMenuItem("Cancel","cancel",Colors::SkyBlue));
	//	setItems(items);
	//	layout(lastPosition,lastSize);
	//}
	//else if (id.compare("show_tutorial") == 0)
	//{
	//	GlobalConfig::tree()->put<bool>("Tutorial.Enabled",true);
	//	vector<string> x;
	//	x.push_back("point_inv");
	//	x.push_back("shake_inv");
	//	LeapDebug::getInstance().setTutorialImages(x);
	//			
	//	vector<RadialMenuItem> items;
	//	items.push_back(RadialMenuItem("Exit and Logout","logout", Colors::DarkRed));
	//	items.push_back(RadialMenuItem("Exit Photo Explorer","exit",Colors::OrangeRed));
	//	items.push_back(RadialMenuItem("Hide Tutorial","hide_tutorial", Colors::DarkTurquoise));
	//	items.push_back(RadialMenuItem("Privacy Information","show_privacy_info", Colors::DeepSkyBlue));
	//	items.push_back(RadialMenuItem("Cancel","cancel",Colors::SkyBlue));
	//	setItems(items);
	//	layout(lastPosition,lastSize);
	//}
	else
	{
		GraphicsContext::getInstance().invokeGlobalAction(id);
		this->dismiss();
	}
	//else if (ItemClickedCallback(id))
	//{
	//	dismiss();
	//}	
}

void RadialMenu::layout(Vector pos, cv::Size2f size)
{	
	static float cornerPadding = GlobalConfig::tree()->get<float>("Menu.CornerPadding");

	this->lastPosition = pos;
	this->lastSize = size;

	if (state == MenuState_DisplayFull)
	{
		float layoutHeight;
		if (!visible)
			layoutHeight = 10;
		else
			layoutHeight = size.height*.12f;

		cv::Size2f childSize = cv::Size2f(size.width * .25f,layoutHeight);

		Vector center = pos+Vector(size.width*.5f,size.height*.5f,0);
		float offset = -2.0f;
		float spacing = childSize.height*1.2f;

		for (auto it = children.begin(); it != children.end(); it++)
		{			
			if ((*it) == privacyInfoBox || (*it) == menuLaunchButton)
				continue;

			(*it)->layout(center + Vector(-childSize.width/2.0f,offset*spacing,0),childSize);
			offset += 1.0f;
		}
		
		cv::Size2f dialogSize = cv::Size2f(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogWidth"),GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogHeight"));

		privacyInfoBox->layout(pos + Vector((size.width - dialogSize.width)*.5f,size.height*1.2f,14), dialogSize);
		
		float height = getMenuHeight();
		cv::Size2f menuButtonSize = cv::Size2f(height*1.5f,height);
		menuLaunchButton->layout(Vector(size.width - menuLaunchButton->getMeasuredSize().width,-height*2.0f,10) + pos,menuButtonSize);		
		menuLaunchButton->layout(Vector(size.width - menuLaunchButton->getMeasuredSize().width,-height*2.0f,10) + pos,menuButtonSize);
	}
	else if (state == MenuState_ShowingDialog)
	{
		cv::Size2f dialogSize = cv::Size2f(GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogWidth"),GlobalConfig::tree()->get<float>("Menu.PrivacyInfo.DialogHeight"));

		privacyInfoBox->layout(pos + Vector(size.width - dialogSize.width,size.height - dialogSize.height,14)*.5f,dialogSize);

	}
	else if (state == MenuState_ButtonOnly) 
	{
		float height = getMenuHeight();

		cv::Size2f menuButtonSize = cv::Size2f(height*1.5f,height);
		menuLaunchButton->layout(Vector(size.width - menuLaunchButton->getMeasuredSize().width,0,10) + pos,menuButtonSize);
		menuLaunchButton->layout(Vector(size.width - menuLaunchButton->getMeasuredSize().width,0,10) + pos,menuButtonSize);
	}
	this->layoutDirty = false;
}

void RadialMenu::show()
{
	state = MenuState_DisplayFull;
	LeapInput::getInstance()->requestGlobalGestureFocus(this);
	GraphicsContext::getInstance().requestExclusiveClarity(this);
	menuLaunchButton->setVisible(false);
	layout(lastPosition,lastSize);
}

void RadialMenu::dismiss()
{
	state = MenuState_ButtonOnly;
	LeapInput::getInstance()->releaseGlobalGestureFocus(this);
	GraphicsContext::getInstance().releaseExclusiveClarity(this);
	menuLaunchButton->setVisible(true);
	layout(lastPosition,lastSize);
}

void RadialMenu::onGlobalGesture(const Controller & controller, std::string gestureType)
{
	if (gestureType.compare("shake") == 0)
	{
		if (state == MenuState_ShowingDialog)
		{
			state = MenuState_DisplayFull;
			layout(lastPosition,lastSize);
		}
		else
			dismiss();
	}
}

bool RadialMenu::onLeapGesture(const Controller & controller, const Gesture & gesture)
{
	return false;
}

void RadialMenu::getTutorialDescriptor(vector<string> & tutorial)
{
	tutorial.push_back("point_inv");
	tutorial.push_back("shake_inv");
}

void RadialMenu::draw()
{	
	if (!GraphicsContext::getInstance().IsBlurCurrentPass)
	{		
		if (state == MenuState_DisplayFull || state == MenuState_ShowingDialog)
		{
			ViewGroup::draw();	
		}
		else if (state == MenuState_ButtonOnly && !GraphicsContext::getInstance().BlurRenderEnabled) 
		{
			menuLaunchButton->draw();
		}			
	}
}


bool RadialMenu::checkMenuOpenGesture(const Gesture & gesture)
{
	if (GlobalConfig::tree()->get<bool>("Menu.CircleGesture.Enabled") && 
		gesture.type() == Gesture::Type::TYPE_CIRCLE && gesture.state() == Gesture::STATE_UPDATE)
	{
		CircleGesture circle(gesture);
		if(	(  
				!GlobalConfig::tree()->get<bool>("Menu.CircleGesture.SinglePointableOnly") || 
				circle.hands().count() == 0 || circle.hands()[0].pointables().count() == 1
			) 
			&& 
			circle.progress() >  GlobalConfig::tree()->get<float>("Menu.CircleGesture.MinRotations") && 
			circle.durationSeconds() <  GlobalConfig::tree()->get<float>("Menu.CircleGesture.MaxDuration") &&
			(circle.normal().angleTo(Leap::Vector::backward()) < (.25f * PI) || 
			circle.normal().angleTo(Leap::Vector::forward()) < PI/4.0f) && 
			circle.radius() > GlobalConfig::tree()->get<float>("Menu.CircleGesture.MinRadius") &&
			circle.radius() < GlobalConfig::tree()->get<float>("Menu.CircleGesture.MaxRadius") 
		)
		{
			return true;
		}	
	}
	return false;
}