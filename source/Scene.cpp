#include "Scene.h"

using namespace G3D::units;


Entity::Entity
	(const std::string& n, const PhysicsFrameSpline& frameSpline, 
	const ArticulatedModel::Ref& artModel, const ArticulatedModel::PoseSpline& artPoseSpline,
	const MD2Model::Ref& md2Model,
	const MD3Model::Ref& md3Model) : GEntity(n, frameSpline, artModel, artPoseSpline, md2Model, md3Model) {
}


Entity::Ref Entity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const ArticulatedModel::Ref& m, const ArticulatedModel::PoseSpline& poseSpline) {
	Entity::Ref e = new Entity(n, frameSpline, m, poseSpline, NULL, NULL);

	// Set the initial position
	e->onSimulation(0, 0);
	return e;
}


Entity::Ref Entity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD2Model::Ref& m) {
	Entity::Ref e = new Entity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), m, NULL);

	// Set the initial position
	e->onSimulation(0, 0);
	return e;
}


Entity::Ref Entity::create(const std::string& n, const PhysicsFrameSpline& frameSpline, const MD3Model::Ref& m) {
	Entity::Ref e = new Entity(n, frameSpline, NULL, ArticulatedModel::PoseSpline(), NULL, m);

	// Set the initial position
	e->onSimulation(0, 0);
	return e;
}


////////////////////////////////////////////////////////////////////////

void Scene::onSimulation(RealTime deltaTime) {
	m_time += deltaTime;
	for (int i = 0; i < m_entityArray.size(); ++i) {
		m_entityArray[i]->onSimulation(m_time, deltaTime);
	}
}


/** Returns a table mapping scene names to filenames */
static Table<std::string, std::string>& filenameTable() {
	static Table<std::string, std::string> filenameTable;

	if (filenameTable.size() == 0) {
		// Create a table mapping scene names to filenames
		Array<std::string> filenameArray;

		FileSystem::ListSettings settings;
		settings.files = true;
		settings.directories = false;
		settings.includeParentPath = true;
		settings.recursive = true;

		FileSystem::list("*.scn.any", filenameArray, settings);

		logLazyPrintf("Found scenes:\n");
		for (int i = 0; i < filenameArray.size(); ++i) {
			Any a;
			try {
				a.load(filenameArray[i]);

				std::string name = a["name"].string();
				alwaysAssertM(! filenameTable.containsKey(name),
					"Duplicate scene names in " + filenameArray[i] + " and " +
					filenameTable["name"]);

				logLazyPrintf("  \"%s\" (%s)\n", name.c_str(), filenameArray[i].c_str());
				filenameTable.set(name, filenameArray[i]);
			} catch (const ParseError& e) {
				logLazyPrintf("  <Parse error at %s:%d(%d): %s>\n", e.filename.c_str(), e.line, e.character, e.message.c_str());
			} catch (...) {
				logLazyPrintf("  <Error while loading %s>\n", filenameArray[i].c_str());
			}
		}
		logPrintf("");
	}

	return filenameTable;
}


Array<std::string> Scene::sceneNames() {
	return filenameTable().getKeys();
}


Scene::Ref Scene::create(const std::string& scene, GCamera& camera) 
{
	if (scene == "") 
	{
		return NULL;
	}

	Scene::Ref s = new Scene();

	const std::string* f = filenameTable().getPointer(scene);
	if (f == NULL) 
	{
		throw "No scene with name '" + scene + "' found in (" + 
			stringJoin(filenameTable().getKeys(), ", ") + ")";
	}
	const std::string& filename = *f;

	Any any;
	any.load(filename);

	// Load the lighting
	s->m_lighting = Lighting::create(any.get("lighting", Lighting::Specification()));

	// Load the models
	Any models = any["models"];
	typedef ReferenceCountedPointer<ReferenceCountedObject> ModelRef;
	Table< std::string, ModelRef > modelTable;
	for (Any::AnyTable::Iterator it = models.table().begin(); it.hasMore(); ++it) {
		ModelRef m;
		Any v = it->value;
		if (v.nameBeginsWith("ArticulatedModel")) {
			m = ArticulatedModel::create(v);
		} else if (v.nameBeginsWith("MD2Model")) {
			m = MD2Model::create(v);
		} else if (v.nameBeginsWith("MD3Model")) {
			m = MD3Model::create(v);
		} else {
			debugAssertM(false, "Unrecognized model type: " + v.name());
		}

		modelTable.set(it->key, m);        
	}

	// Instance the models
	Any entities = any["entities"];
	for (Table<std::string, Any>::Iterator it = entities.table().begin(); it.hasMore(); ++it) {
		const std::string& name = it->key;
		const Any& modelArgs = it->value;

		modelArgs.verifyType(Any::ARRAY);
		const ModelRef* model = modelTable.getPointer(modelArgs.name());
		modelArgs.verify((model != NULL), 
			"Can't instantiate undefined model named " + modelArgs.name() + ".");

		PhysicsFrameSpline frameSpline;
		ArticulatedModel::PoseSpline poseSpline;
		if (modelArgs.size() >= 1) {
			frameSpline = modelArgs[0];
			if (modelArgs.size() >= 2) {
				// Poses 
				poseSpline = modelArgs[1];
			}
		}

		ArticulatedModel::Ref artModel = model->downcast<ArticulatedModel>();
		MD2Model::Ref         md2Model = model->downcast<MD2Model>();
		MD3Model::Ref         md3Model = model->downcast<MD3Model>();

		if (artModel.notNull()) {
			s->m_entityArray.append(Entity::create(name, frameSpline, artModel, poseSpline));
		} else if (md2Model.notNull()) {
			s->m_entityArray.append(Entity::create(name, frameSpline, md2Model));
		} else if (md3Model.notNull()) {
			s->m_entityArray.append(Entity::create(name, frameSpline, md3Model));
		}
	}

	// Load the camera
	camera = any["camera"];

	if (any.containsKey("skybox")) {
		Any sky = any["skyBox"];
		s->m_skyBoxConstant = sky.get("constant", 1.0f);
		if (sky.containsKey("texture")) {
			s->m_skyBoxTexture = Texture::create(sky["texture"]);
		}
	} else {
		s->m_skyBoxTexture = s->m_lighting->environmentMapTexture;
		s->m_skyBoxConstant = s->m_lighting->environmentMapConstant;
	}

	// Default to using the skybox as an environment map if none is specified.
	if (s->m_skyBoxTexture.notNull() && s->m_lighting->environmentMapTexture.isNull()) {
		s->m_lighting->environmentMapTexture  = s->m_skyBoxTexture;
		s->m_lighting->environmentMapConstant = s->m_skyBoxConstant;
	}

	return s;
}


void Scene::onPose(Array<Surface::Ref>& surfaceArray) {
	for (int e = 0; e < m_entityArray.size(); ++e) {
		m_entityArray[e]->onPose(surfaceArray);
	}
}
