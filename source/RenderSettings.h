#ifndef RenderSettings_h
#define RenderSettings_h

#include <G3D/G3DAll.h>

class RenderSettings
{
protected:
	GuiDropDownList* _resolutionList;
public:
	int _width;
	int _height;
	bool _multiThreaded;
	bool _useTriTree;
	bool _shadowCast;
	int _backwardBounces;
	bool _usePhotonMap;
	bool _useDirectShading;
	float _photonGatherRadius;
	int _numberOfPhotons;
	int _forwardBounces;

	bool _debugPixelTrace;

	bool _useSuperBSDF;

	RenderSettings();
	virtual ~RenderSettings() {}

	void onResolutionChange();

	void makeGui(GuiPane* p);
};


#endif