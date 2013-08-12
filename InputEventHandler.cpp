#include "InputEventHandler.hpp"


InputEventHandler::InputEventHandler()
{

}

void InputEventHandler::init(GLFWwindow * eventWindow)
{
	glfwSetCharCallback(eventWindow,InputEventHandler::glfwCharCallback);
	glfwSetWindowFocusCallback(eventWindow,InputEventHandler::glfwFocusCallback);
}

void InputEventHandler::addUnicodeCharacterListener(boost::function<bool(GLFWwindow*,unsigned int)> unicodeCallback)
{
	unicodeCallbacks.push_back(unicodeCallback);
}

void InputEventHandler::onCharEvent(GLFWwindow * window, unsigned int key)
{
	for (auto it = unicodeCallbacks.begin(); it != unicodeCallbacks.end();it++)
	{
		bool handled = (*it)(window,key);
		if (handled)
			break;
	}
}

void InputEventHandler::glfwCharCallback(GLFWwindow * window, unsigned int key)
{
	InputEventHandler::getInstance().onCharEvent(window,key);
}


void InputEventHandler::addFocusChangedCallback(boost::function<bool(GLFWwindow*,int)> focusChanged)
{
	focusChangedCallbacks.push_back(focusChanged);
}

void InputEventHandler::onFocusEvent(GLFWwindow * window, int focused)
{
	for (auto it = focusChangedCallbacks.begin(); it != focusChangedCallbacks.end();it++)
	{
		bool handled = (*it)(window,focused);
		if (handled)
			break;
	}
}

void InputEventHandler::glfwFocusCallback(GLFWwindow * window, int focused)
{
	InputEventHandler::getInstance().onFocusEvent(window,focused);
}