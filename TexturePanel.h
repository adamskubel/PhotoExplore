#include "PanelBase.h"

#ifndef TEXTURE_PANEL_H_
#define TEXTURE_PANEL_H_

class TexturePanel : public PanelBase {

	
public:
	
	enum AlignmentMode{

		Left,
		Center,
		Right
	};

protected:
	TexturePanel();

	int textureScaleMode;
	AlignmentMode horizontalAlignment;
	AlignmentMode verticalAlignment;

	GLuint currentTextureId;
	float textureWidth,textureHeight;
	bool allowSubPixelRendering;

	Color textureTint;

	void drawTexture(GLuint texId, Vector drawPosition, float drawWidth, float drawHeight);
	
	virtual void drawContent(Vector drawPosition, float drawWidth, float drawHeight);

public:

	TexturePanel(GLuint textureId)
	{
		currentTextureId = textureId;
	}

	Vector textureOffset, textureScale;
	void setTextureWindow(Leap::Vector textureOffset, Leap::Vector scaleVector);
	void setScaleMode(int scaleMode);

	void setAllowSubPixelRendering(bool allowSubPixel);
	bool getAllowSubPixelRendering();

	virtual void getBoundingArea(Vector & position, float & drawWidth, float & drawHeight);

	void setTextureTint(Color textureTint);
	Color getTextureTint();

	void setTextureAlpha(float textureAlpha);
	float getTextureAlpha();


};

#endif

