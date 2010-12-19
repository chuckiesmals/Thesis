#include "App.h"
#include "Scene.h"
#include "RenderSettings.h"
#include "Raytracer.h"

int numberOfTris = 0;
double elapsedTime = 0;

App::App(const GApp::Settings& settings) : GApp(settings), _wireframe(false)
{
}

void App::invokePhotonMap()
{
	_raytracer.generatePhotonMap(_renderSettings);
}

void App::invokeRender()
{
	if (_scene.notNull())
	{
		drawMessage("Rendering");
		RealTime start = System::time();
		Image3::Ref renderedImage = _raytracer.render(_renderSettings, defaultCamera);
		RealTime end = System::time();

		if (renderedImage == NULL)
		{
			return;
		}

		elapsedTime = end - start;

		if (_renderWindow.notNull())
		{
			removeWidget(_renderWindow);
		}

		//_renderWindow = GuiWindow::create("Rendered Image", NULL, Rect2D::xywh(200, 200, _renderSettings._width * 1.1f, _renderSettings._height * 1.2f));
		_renderWindow = GuiWindow::create("Rendered Image", NULL, Rect2D::xywh(200, 200, 400, 400));
		addWidget(_renderWindow);
		_renderWindow->pane()->addTextureBox(
			Texture::fromMemory(
				"render", 
				renderedImage->getCArray(), 
				renderedImage->format(), 
				renderedImage->width(),
				renderedImage->height(),
				1));

		_renderWindow->pane()->addNumberBox("", &elapsedTime);
		_renderWindow->setVisible(true);
	}
}

void App::reloadScene()
{
	loadScene();
}

void App::onInit() 
{
	_wireframe = true;
	_scene = Scene::create(_currentScene, defaultCamera);

	debugPane->addCheckBox("Wireframe", &_wireframe);

	_sceneDropDownList = debugPane->addDropDownList("Scene", Scene::sceneNames(), NULL, GuiControl::Callback(this, &App::loadScene));

	debugPane->addLabel("Number of triangles: ");
	debugPane->addNumberBox("", &numberOfTris);
	debugPane->addButton("Reload Scene", this, &App::reloadScene);

	debugPane->addLabel("Raytracer");

	_renderSettings.makeGui(debugPane);
	debugPane->addButton("Create Photon Map", this, &App::invokePhotonMap);
	debugPane->addButton("Render Scene", this, &App::invokeRender);

	debugPane->addButton("Exit", this, &App::endProgram);
	debugPane->setWidth(500);
	debugPane->setVisible(true);
	debugPane->setEnabled(true);

}

void App::onPose(Array<Surface::Ref>& surfaceArray, Array<Surface2D::Ref>& surface2D)
{
	(void)surface2D;
	if (_scene.notNull()) 
	{
		_scene->onPose(surfaceArray);
	}
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	(void)idt; (void)sdt; (void)rdt;
	// Add physical simulation here.  You can make your time
	// advancement based on any of the three arguments.
	if (_scene.notNull()) 
	{
		_scene->onSimulation(sdt);
	}
}

void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) 
{
	// Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
	Surface2D::sortAndRender(rd, posed2D);
}

void App::onGraphics3D (RenderDevice* rd, Array<Surface::Ref>& surface3D)
{
	if (_scene.isNull()) 
	{
		return;
	}

	Draw::skyBox(rd, _scene->skyBoxTexture(), _scene->skyBoxConstant());

	Surface::sortAndRender(rd, defaultCamera, surface3D, _scene->lighting());

	if (_wireframe)
	{
		rd->pushState();
		{
			rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
			rd->setLineWidth(2);
			rd->setColor(Color3::black());
			Surface::sendGeometry(rd, surface3D);
		}
		rd->popState();
	}

	Draw::axes(CoordinateFrame(Vector3(0, 0, 0)), rd);

	Draw::lighting(_scene->lighting(), rd);

	if (_renderSettings._usePhotonMap && _raytracer._photonMap)
	{
		rd->setPointSize(5);
		rd->beginPrimitive(PrimitiveType::POINTS);
		for (PhotonGrid::Iterator it = _raytracer._photonMap->begin(); it.hasMore(); ++it)
		{
			rd->setColor(it->power / it->power.max());
			rd->sendVertex(it->position);
		}
		rd->endPrimitive();
	}

	// Call to make the GApp show the output of debugDraw
	drawDebugShapes();
}

void App::onUserInput(UserInput* userInput)
{
	if (_renderSettings._debugPixelTrace && userInput->keyReleased(GKey::LEFT_MOUSE))
	{
		Vector2int16 pixel(userInput->mouseXY().x, userInput->mouseXY().y);
		_raytracer.render(_renderSettings, defaultCamera, pixel);
	}
}

void App::endProgram()
{
	m_endProgram = true;
}

void App::loadScene() {
	const std::string& sceneName = _sceneDropDownList->selectedValue().text();

	// Use immediate mode rendering to force a simple message onto the screen
	drawMessage("Loading " + sceneName + "...");

	// Load the scene
	try 
	{
		_scene = Scene::create(sceneName, defaultCamera);
		numberOfTris = _raytracer.setScene(_scene);
        defaultController->setFrame(defaultCamera.coordinateFrame());

	} 
	catch (const ParseError& e) 
	{
		const std::string& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
		drawMessage(msg);
		debugPrintf("%s", msg.c_str());
		System::sleep(5);
		_scene = NULL;
	}
}