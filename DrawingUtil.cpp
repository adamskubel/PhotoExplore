#include "DrawingUtil.hpp"

using namespace Leap;

void DrawingUtil::drawCircleLine(Vector drawPoint, Color lineColor, float lineWidth, float outerRadius, float startAngle, float endAngle)
{
	float z1 =drawPoint.z;
	float angleRange = endAngle - startAngle;
	
	int vertices = ceilf(60 * angleRange/(Leap::PI*2.0f));
	float anglePerVertex = angleRange/(float)vertices;
	
//	if (angleRange >= Leap::PI*2.0f)
//		vertices++;
	
	float radii [] = {outerRadius - lineWidth*0.5f,outerRadius, outerRadius + lineWidth*0.5f};
	float alphaScale = lineColor.colorArray[3];
	
	float * lineColors [] = {
		lineColor.withAlpha(0*alphaScale).getFloat(),
		lineColor.withAlpha(alphaScale).getFloat(),
		lineColor.withAlpha(0*alphaScale).getFloat(),
	};
	
	glTranslatef(drawPoint.x,drawPoint.y,0);
	for (int lap = 0; lap < 2; lap++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int v=0;v<=vertices;v++)
		{
			float angle = (float)v*anglePerVertex;
			angle += startAngle;
			
			for (int i=0; i < 2; i++)
			{
				float rad = radii[i+lap];
				glColor4fv(lineColors[i+lap]);
				glVertex3f(cosf(angle)*rad,sinf(angle)*rad,z1+ ((float)i * .1f));
			}
		}
		glEnd();
	}
	glTranslatef(-drawPoint.x,-drawPoint.y,0);
	
}

void DrawingUtil::drawCircleFill(Vector drawPoint,Color fillColor, float innerRadius, float outerRadius, float startAngle, float endAngle)
{
	drawCircleFill(drawPoint,fillColor,fillColor,innerRadius,outerRadius,startAngle,endAngle);
}

void DrawingUtil::drawCircleFill(Vector drawPoint,Color innerColor, Color outerColor, float innerRadius, float outerRadius, float startAngle, float endAngle)
{
	float z1 = drawPoint.z;
	if (outerRadius == 0)
		return;
	
	float angleRange = endAngle - startAngle;
	
	int vertices = ceilf(40 * angleRange/(Leap::PI*2.0f));
	
	float anglePerVertex = angleRange/(float)vertices;
	
	glBindTexture( GL_TEXTURE_2D, NULL);
	glLineWidth(0);
	
	glTranslatef(drawPoint.x,drawPoint.y,0);
	glBegin(GL_TRIANGLE_STRIP);
	for (float v=0;v<=vertices;v++)
	{
		float angle = v*anglePerVertex;
		angle += startAngle;
		
		glColor4fv(innerColor.getFloat());
		glVertex3f(cosf(angle)*innerRadius,sinf(angle)*innerRadius,z1);
		glColor4fv(outerColor.getFloat());
		glVertex3f(cosf(angle)*outerRadius,sinf(angle)*outerRadius,z1);
	}
	glEnd();
	glTranslatef(-drawPoint.x,-drawPoint.y,0);
	
	
}
