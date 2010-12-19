#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include "Scene.h"
#include "Raytracer.h"

/**
	\brief Application specific class based on GApp
*/
class App : public GApp 
{
public:
	App (const GApp::Settings& settings);
	
	//! Creation of Scene and allocation of resources occurs here
	virtual void onInit();
	//! Updating entity positions handled here
	virtual void onPose (Array<Surface::Ref>& surface3D, Array<Surface2D::Ref>& surface2D);

	virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

	virtual void onUserInput(UserInput* userInput);

	//! Drawing is handled here
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);
	virtual void onGraphics3D (RenderDevice* rd, Array<Surface::Ref>& surface3D);	

	virtual void endProgram();
private:
	GuiDropDownList* _sceneDropDownList;
	GuiDropDownList* _raycastResolutionList;
	std::string _currentScene;
	Scene::Ref _scene;
	bool _wireframe;
	void loadScene();
	void invokeRender();
	void invokePhotonMap();
	void reloadScene();

	GuiWindow::Ref _renderWindow;

	Array<std::string> _raycastResolutions;

	RenderSettings _renderSettings;
	Raytracer _raytracer;
};

#endif