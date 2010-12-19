#include "Raytracer.h"

void check(float x)
{
	if (x < 0)
	{
		int ohgod = 3498;
	}
}

Raytracer::Raytracer()
{
	_defaultColor = Color3(0, 216.0f/255.0f, 1.0f);
	_photonMap = new PhotonGrid(0.1f);
}

int Raytracer::setScene(const Scene::Ref& scene)
{
	_currentScene = scene;

	Array<Surface::Ref> surfaceArray;
	scene->onPose(surfaceArray);

	_triTree.setContents(surfaceArray, COPY_TO_CPU);
	return _triTree.size();
}

void Raytracer::generatePhotonMap(const RenderSettings& settings)
{
	_settings = settings;
	_photonMap->clear();
	photonMapForwardTrace();
}

int gStatus = 0;
int gNumPixelsRendered = 0;
Image3::Ref Raytracer::render(const RenderSettings& settings, const GCamera& camera, Vector2int16 pixel)
{
	gStatus = 0;
	gNumPixelsRendered = 0;

	_renderedImage = Image3::createEmpty();
	_renderedImage->resize(settings._width, settings._height);
	_currentCamera = camera;
	_settings = settings;
	
	_photonMap->clear();

	if (settings._usePhotonMap)
	{
		if (_photonMap->size() == 0)
		{
			photonMapForwardTrace();
		}
	}

	if (pixel.x == ALL && pixel.y == ALL)
	{
		if (settings._multiThreaded)
		{
			GThread::runConcurrently2D(
				Vector2int32(0,0), 
				Vector2int32(settings._width, settings._height), 
				this, 
				&Raytracer::backwardTrace);
		}
		else
		{
			for (int y = 0; y < settings._height; ++y)
			{
				for (int x = 0; x < settings._width; ++x)
				{
					backwardTrace(x, y);
				}
			}
		}
	}
	else
	{
		backwardTrace(pixel.x, pixel.y);
	}

	return _renderedImage;
}

//=================================
//	Forward Trace
//=================================
void Raytracer::photonMapForwardTrace()
{
	const int numberOfBounces = _settings._forwardBounces;
	const int numberOfPhotons = _settings._numberOfPhotons;

	// Go through all lights in scene
	for (int i = 0; i < _currentScene->lighting()->lightArray.length(); ++i)
	{
		GLight& light = _currentScene->lighting()->lightArray[i];
		
		const bool isSpotLight = false;

		if (!isSpotLight)
		{
			// create hemisphere of light rays and bounce, bounce, bounce
			emitLightPhotons(light, numberOfBounces, numberOfPhotons);
		}
	}
}

void Raytracer::emitLightPhotons(const GLight& light, const int bouncesLeft, const int numberOfPhotons)
{
	const Power3 powerPerPhoton = light.color / numberOfPhotons;
	for (int i = 0; i < numberOfPhotons; ++i)
	{
		// http://mathworld.wolfram.com/SpherePointPicking.html

		float x1 = _random.uniform(-1, 1);
		float x2 = _random.uniform(-1, 1);

		while (x1*x1 + x2*x2 >= 1)
		{
			x1 = _random.uniform(-1, 1);
			x2 = _random.uniform(-1, 1);
		}

		const float x = 2 * x1 * sqrt(1 - x1*x1 - x2*x2);	// (9)
		const float y = 2 * x2 * sqrt(1 - x1*x1 - x2*x2);	// (10)
		const float z = 1 - 2 * (x1*x1 + x2*x2);			// (11)

		const Vector3 direction = Vector3(x,y,z).direction();

		Photon* photon = new Photon(light.position.xyz(), -direction, powerPerPhoton);
		forwardTrace(photon, bouncesLeft, numberOfPhotons);
	}
}

void Raytracer::forwardTrace(Photon* photon, const int bouncesLeft, const int numberOfPhotons) 
{
	if (bouncesLeft > 0)
	{
		Tri::Intersector intersector;
		float distance = inf();
		Ray photonRay(photon->position, photon->direction);

		if (_triTree.intersectRay(photonRay, intersector, distance))
		{
			Color3 pixelColor;
			Vector3 position, normal;
			Vector2 texCoord;
			intersector.getResult(position, normal, texCoord);
			photon->position = position + normal * 0.0001f;

			Photon* photonCopy = new Photon(photon);
			//if (bouncesLeft != _settings._forwardBounces)
			{
				_photonMap->insert(photon);
			}

			SurfaceSample surfaceSample(intersector);
			if (scatter(intersector, photonCopy))
			{
				forwardTrace(photonCopy, bouncesLeft - 1, numberOfPhotons);
			}
		}
	}
}

bool Raytracer::scatter(const SurfaceSample& s, Photon* p) const
{
	bool scatter = false;
	float t = _random.uniform(0, 1);

	// Check Impulses
	SuperBSDF::Ref bsdf = s.material->bsdf();
	SmallArray<SuperBSDF::Impulse, 3> impulseArray;
	bsdf->getImpulses(s.shadingNormal, s.texCoord, p->direction, impulseArray); // todo: check p.direction

	Color3 impulseSum;

	for (int i = 0; i < impulseArray.size(); ++i)
	{
		if (impulseArray[i].coefficient.average() < 0)
		{
			int sures = 234;
		}
		
		//t -= impulseArray[i].coefficient.average();
		impulseSum += impulseArray[i].coefficient;

		if (t - impulseArray[i].coefficient.average() < 0)
		{
			scatter = true;
			p->direction = impulseArray[i].w;
			p->power = p->power * t / impulseArray[i].coefficient.average();
			check(p->power.average());
			break;
		}
	}

	// Check Diffuse
	if (!scatter)
	{
		const Component4 lambertian = bsdf->lambertian();
		//t - (s.lambertianReflect * (1 - impulseSum.average())).average();

		if (t - (s.lambertianReflect * (1 - impulseSum.average())).average() < 0)
		{
			scatter = true;
			p->direction = Vector3::cosHemiRandom(s.shadingNormal);
			//p->power = p->power * (t / (s.lambertianReflect * (1 - impulseSum.average())).average());
			p->power = p->power * (t / (s.lambertianReflect.average()));// * (1 - impulseSum.average())).average());
			check(p->power.average());
		}
	}

	return scatter;
}

//=================================
//	Backward Trace - Shading
//=================================
void Raytracer::backwardTrace(int x, int y)
{
	Color3 pixelColor = _defaultColor;
	Ray ray;
	
	if (_settings._debugPixelTrace)
	{
		ray = _currentCamera.worldRay(x+0.5f, y+0.5f, Rect2D::xywh(0, 0, 1440, 800));
	}
	else
	{
		ray = _currentCamera.worldRay(x+0.5f, y+0.5f, Rect2D::xywh(0, 0, _settings._width, _settings._height));
	}

	pixelColor = backwardTrace(ray, _settings._backwardBounces);
	
	if (!_settings._debugPixelTrace)
	{
		_renderedImage->set(x, y, pixelColor);
	}

	gNumPixelsRendered++;

	if (gNumPixelsRendered / (_settings._width * _settings._height) > 0.2f && gStatus < 2)
	{
		printf("20%%\n");
		gStatus = 2;
	}
	else if (gNumPixelsRendered / (_settings._width * _settings._height) > 0.4f && gStatus < 4)
	{
		printf("40%%\n");
		gStatus = 4;
	}
	else if (gNumPixelsRendered / (_settings._width * _settings._height) > 0.6f && gStatus < 6)
	{
		printf("60%%\n");
		gStatus = 6;
	}
	else if (gNumPixelsRendered / (_settings._width * _settings._height) > 0.8f && gStatus < 8)
	{
		printf("80%%\n");
		gStatus = 8;
	}

}

Radiance3 Raytracer::backwardTrace(const Ray& ray, int backwardBouncesLeft) const
{
	Color3 pixelColor;
	Tri::Intersector intersector;
	float distance = inf();

	// use tri tree
	if (_settings._useTriTree)
	{
		if (_triTree.intersectRay(ray, intersector, distance))
		{
			pixelColor = shadePixel(intersector, ray, backwardBouncesLeft);
		}
	}
	// or just go through array
	else
	{
		distance = inf();
		for (int i = 0; i < _triTree.size(); ++i)
		{
			intersector(ray, _triTree[i], distance);
			if (distance < inf())
			{
				pixelColor = shadePixel(intersector, ray, backwardBouncesLeft);
			}
		}
	}
	return pixelColor;
}

Color3 Raytracer::shadePixel(const Tri::Intersector& intersector, const Ray& ray, int backwardBouncesLeft) const
{
	Color3 pixelColor;
	Vector3 position, normal;
	Vector2 texCoord;
	intersector.getResult(position, normal, texCoord);
	SurfaceSample surfaceSample(intersector);

	if (_settings._useSuperBSDF)
	{
		if(_settings._useDirectShading)
		{
			pixelColor = shadeDirectBSDF(surfaceSample, ray);
		}
		
		pixelColor += shadeSpecularBSDF(surfaceSample, -ray.direction(), backwardBouncesLeft);
		
		if (_settings._usePhotonMap)
		{
			pixelColor += shadeIndirectIllumination(surfaceSample);
		}
						
	}
	else
	{
		pixelColor = myShadePixel(intersector, ray, backwardBouncesLeft);
	}

	return pixelColor;
}

bool Raytracer::isInSpotlight(const GLight& light, const Vector3 w_i) const
{
	bool returnValue = false;

	// Spotlight
	const bool firstTerm = light.spotHalfAngle < halfPi();

	const float lightIn_dot_spotDirection = -w_i.dot(light.spotDirection);
	const float cosSpotHalfAngle = cosf(light.spotHalfAngle);
	const bool secondTerm = lightIn_dot_spotDirection < cosSpotHalfAngle;
	
	const bool outsideSpotLight = firstTerm && secondTerm;

	if (!outsideSpotLight)
	{
		returnValue = true;
	}

	return returnValue;
}

bool Raytracer::isInShadow(const SurfaceSample& intersector, const Vector3 w_i, const float lightDistance) const
{
	bool returnValue = false;

	if (_settings._shadowCast)
	{
		// Check if in Shadow, cast ray backwards, up to distance to light....
		Ray rayToLight(intersector.shadingLocation + (intersector.shadingNormal * 0.0001f), (w_i.unit()));
		Tri::Intersector shadowIntersector;
		float shadowDistance = lightDistance;
		if (_triTree.intersectRay(rayToLight, shadowIntersector, shadowDistance))
		{
			returnValue = true;
		}
	}

	return returnValue;
}

Color3 Raytracer::myShadePixel(const Tri::Intersector& intersector, const Ray& ray, int backwardBouncesLeft) const
{
	ray; backwardBouncesLeft;
	Vector3 position, normal;
	Vector2 texCoord;
	intersector.getResult(position, normal, texCoord);
	SurfaceSample surfaceSample(intersector);

	Color3 lightContribution(0,0,0);
	for (int i = 0; i < _currentScene->lighting()->lightArray.length(); ++i)
	{
		GLight& light = _currentScene->lighting()->lightArray[i];

		const float lightDistance = light.distance(position);
		const Power3 power = light.power().rgb();
		check(power.average());
		const float lightArea = 4 * pif() * lightDistance * lightDistance;
		const Color3 kx = surfaceSample.lambertianReflect;

		const Vector3 light_in = (light.position.xyz() - position).unit();
		const float lightIn_dot_normal = light_in.dot(normal);

		// Spotlight
		if (!isInSpotlight(light, light_in))
		{
			continue;
		}

		if (isInShadow(surfaceSample, light_in, lightDistance))
		{
			continue;
		}

		Color3 fx(0, 0, 0);
		if (lightIn_dot_normal > 0)
		{
			fx = kx / pif();
		}

		lightContribution += (power / lightArea) * lightIn_dot_normal * fx;
	}

	return lightContribution;
}

Radiance3 Raytracer::shadeSpecularBSDF(const SurfaceSample& intersector, const Vector3& w_o, int backwardBounces) const
{
	Radiance3 L_r;

	if (backwardBounces > 1)
	{
		SuperBSDF::Ref bsdf = intersector.material->bsdf();
		SmallArray<SuperBSDF::Impulse, 3> impulseArray;

		bsdf->getImpulses(intersector.shadingNormal, intersector.texCoord, w_o, impulseArray);

		for (int i = 0; i < impulseArray.size(); ++i)
		{
			const SuperBSDF::Impulse& impulse = impulseArray[i];
			Ray ray(intersector.shadingLocation, impulse.w);

			Radiance3 L_in = backwardTrace(ray, backwardBounces-1);

			L_r += L_in * impulse.coefficient;
		}
	}
	return L_r;
}

Color3 Raytracer::shadeDirectBSDF(const SurfaceSample& intersector, const Ray& ray) const
{	
	Color3 lightContribution(0,0,0);
	for (int i = 0; i < _currentScene->lighting()->lightArray.length(); ++i)
	{
		GLight& light = _currentScene->lighting()->lightArray[i];
		const Point3& X = intersector.shadingLocation;
		const Point3& n = intersector.shadingNormal;
		const Vector3& diff = light.position.xyz() - X;
		const Vector3 w_i = diff.direction();
		const float distance = diff.length();
		const Vector3 w_eye = -ray.direction();
		const Power3& Phi = light.color.rgb();

		if (!isInSpotlight(light, w_i))
		{
			continue;
		}

		if (isInShadow(intersector, w_i, distance))
		{
			continue;
		}

		// power area
		const Color3& E_i = w_i.dot(n) * Phi / (4 * pif() * (distance*distance));

		const SuperBSDF::Ref bsdf = intersector.material->bsdf();
		const Color3& bsdfColor = bsdf->evaluate(intersector.shadingNormal, intersector.texCoord, w_i, w_eye).rgb();

		lightContribution += bsdfColor * E_i + intersector.emit;
	}

	return lightContribution;
}

float k (float x, float radius) 
{
	return (1 - (x / radius));
}

Radiance3 Raytracer::shadeIndirectIllumination(const SurfaceSample& intersector) const
{
	if (_settings._debugPixelTrace)
	{
		int suresh = 1374;
	}
	
	Radiance3 accumulatedPhotonRadiance;
	
	const Point3& X = intersector.shadingLocation;
	const Point3& n = intersector.shadingNormal;
	const float radius = _settings._photonGatherRadius;
	
	const SuperBSDF::Ref bsdf = intersector.material->bsdf();
	
	static bool increaseGathering = true;

	if (increaseGathering)
	{
		const int gatheredPhotons = 5000;
		int numberOfPhotons = 0;
		float increasingRadius = 0.0f;
		static float increasingRate = 0.05f;
		
		while (numberOfPhotons < gatheredPhotons && increasingRadius < radius)
		{
			// Gather all photons within radius of intersection
			const Sphere sphere(X, increasingRadius);
			PhotonGrid::SphereIterator it = _photonMap->beginSphereIntersection(sphere);
			while (it.hasMore())
			{
				numberOfPhotons++;
				++it;
			}
			increasingRadius += increasingRate;
		}

		const Sphere sphere(X, increasingRadius);
		PhotonGrid::SphereIterator it = _photonMap->beginSphereIntersection(sphere);

		//printf("%4.2f\n", increasingRadius);

		while (it.hasMore())
		{
			const Vector3& diff = it->position.xyz() - X;
			const Vector3 w_i = diff.direction();
			const Vector3 w_eye = -(it->direction);
			const Color3& bsdfColor = bsdf->evaluate(intersector.shadingNormal, intersector.texCoord, w_i, w_eye).rgb();
		
			Radiance3 photo_illum = (it->power / ((pif() * increasingRadius*increasingRadius)/3)) * k((it->position - X).magnitude(), increasingRadius);
			photo_illum = photo_illum * max(0.0f, (it->direction * -1).dot(n));
			photo_illum = photo_illum * bsdfColor;
			accumulatedPhotonRadiance += photo_illum;
			++it;
		}
	}
	else
	{
		// Gather all photons within radius of intersection
		const Sphere sphere(X, radius);
		PhotonGrid::SphereIterator it = _photonMap->beginSphereIntersection(sphere);
		while (it.hasMore())
		{
			const Vector3& diff = it->position.xyz() - X;
			const Vector3 w_i = diff.direction();
			const Vector3 w_eye = -(it->direction);
			const Color3& bsdfColor = bsdf->evaluate(intersector.shadingNormal, intersector.texCoord, w_i, w_eye).rgb();
	
			Radiance3 photo_illum = (it->power / ((pif() * radius*radius)/3)) * k((it->position - X).magnitude(), radius);
			photo_illum = photo_illum * max(0.0f, (it->direction * -1).dot(n));
			photo_illum = photo_illum * bsdfColor;
			accumulatedPhotonRadiance += photo_illum;
			++it;
		}
	}

	// Sum their power
	return accumulatedPhotonRadiance;
}
