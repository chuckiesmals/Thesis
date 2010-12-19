#include "RenderSettings.h"

RenderSettings::RenderSettings() : 
	_width(256), 
	_height(256), 
	_multiThreaded(true), 
	_useTriTree(true), 
	_shadowCast(true), 
	_backwardBounces(1), 
	_debugPixelTrace(false),	
	_useSuperBSDF(true),
	_usePhotonMap(true),
	_useDirectShading(false),
	_photonGatherRadius(0.4f),
	_forwardBounces(6),
	_numberOfPhotons(1000000)
{

}

void RenderSettings::onResolutionChange()
{
	TextInput ti(TextInput::FROM_STRING, _resolutionList->selectedValue().text());

	_width = ti.readNumber();
	ti.readSymbol("x");
	_height = ti.readNumber();
}

void RenderSettings::makeGui(GuiPane* pane)
{
	pane->setNewChildSize(250, GuiPane::DEFAULT_SIZE, 120);

	static const Array<std::string> resArray
		("256 x 256",
		"320 x 200",
		"640 x 480",
		"1280 x 1024");

	_resolutionList = pane->addDropDownList(
		"Resolution", 
		resArray, 
		NULL, 
		GuiControl::Callback(this, &RenderSettings::onResolutionChange));

	pane->addCheckBox("Use Tri Tree", &_useTriTree);
	pane->addCheckBox("Threaded", &_multiThreaded);

	pane->addCheckBox("Use SuperBSDF", &_useSuperBSDF);
	pane->addCheckBox("Cast Shadows", &_shadowCast);

	pane->addCheckBox("Use Photon Mapping", &_usePhotonMap);
	pane->addCheckBox("Use Direct Shading", &_useDirectShading);
	pane->addNumberBox("Number of Photons", &_numberOfPhotons);
	pane->addNumberBox("Forward Bounces", &_forwardBounces);
	
	pane->addNumberBox("Gather Radius", &_photonGatherRadius);
	
	GuiSlider<int>* floatSlider = pane->addSlider("Recursive Bounces", &_backwardBounces, 1, 10);
	floatSlider->setWidth(200);

	pane->addNumberBox("", &_backwardBounces);


	pane->addCheckBox("Debug Pixel Trace", &_debugPixelTrace);
	

}