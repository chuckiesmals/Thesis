#ifndef Raytracer_h
#define Raytracer_h

#include <G3D/G3DAll.h>
#include "Scene.h"
#include "RenderSettings.h"

class Photon
{
public:
	Vector3 direction;
	Point3 position;
	Power3 power;

	//static Photon::Ref create()
	//{
	//	Photon::Ref photon = new Photon();
	//	return photon;
	//}

	//static Photon::Ref create(const Point3& position, const Vector3& direction, const Power3& power)
	//{
	//	Photon::Ref photon = new Photon();
	//	photon->position = position;
	//	photon->direction = direction;
	//	photon->power = power;
	//	return photon;
	//}

	//static Photon::Ref create(const Photon::Ref photonToCopy)
	//{
	//	Photon::Ref photon = new Photon();
	//	photon->position = photonToCopy->position;
	//	photon->power = photonToCopy->power;
	//	photon->direction = photonToCopy->direction;
	//	return photon;
	//}

	Photon(const Photon* photon)
	{
		this->position = photon->position;
		this->direction = photon->direction;
		this->power = photon->power;
	}

	Photon(const Point3& position, const Vector3& direction, const Power3& power)
	{
		this->position = position;
		this->direction = direction;
		this->power = power;
	}

	bool operator==(const Photon& other) const
	{
		return (position == other.position);
	}

	static void getPosition(const Photon& p, Vector3& pos)
	{
		pos = p.position;
	}

	Photon()
	{
	
	}
};

typedef PointHashGrid<Photon, Photon> PhotonGrid;

class Raytracer
{
private:
	Color3 _defaultColor;
	GCamera _currentCamera;
	RenderSettings _settings;
	Image3::Ref _renderedImage;
	TriTree _triTree;
	Scene::Ref _currentScene;
	
	// Forward Tracing (photon mapping)
	void photonMapForwardTrace();
	void emitLightPhotons(const GLight& light, const int bouncesLeft, const int numberOfPhotons);
	void forwardTrace(Photon* photon, const int bouncesLeft, const int numberOfPhotons);
	bool scatter(const SurfaceSample& s, Photon* p) const;
	
	// Backward Tracing
	Color3 shadePixel(const Tri::Intersector& intersector, const Ray& ray, int backwardBouncesLeft) const;
	Color3 shadeDirectBSDF(const SurfaceSample& intersector, const Ray& ray) const;
	Radiance3 shadeSpecularBSDF(const SurfaceSample& intersector, const Vector3& w_o, int backwardBounces) const;
	Radiance3 shadeIndirectIllumination(const SurfaceSample& intersector) const;

	Color3 myShadePixel(const Tri::Intersector& intersector, const Ray& ray, int backwardBouncesLeft) const;
	
	bool isInSpotlight(const GLight& light, const Vector3 w_i) const;
	bool isInShadow(const SurfaceSample& intersector, const Vector3 w_i, const float lightDistance) const;

	mutable Random _random;
protected:
	/** Trace this pixel only and write the result to _image. Threadsafe. */
	void backwardTrace(int x, int y);

	/** Estimates \f$L_{\mathrm{i}}(X, \hat{\omega}_{\mathrm{i}})\f$
	for the ray, where \f$\hat{\omega}_{\mathrm{i}}\f$ =
	<code>-ray.direction()</code>.
	\param backwardBouncesLeft Reserved for future use.
	*/
	Radiance3 backwardTrace(const Ray& ray, int backwardBouncesLeft) const;

public:
	mutable PhotonGrid* _photonMap;

	Raytracer();
	virtual ~Raytracer() {}
	enum {ALL = -1};

	/** Returns the number of triangles in the scene */
	int setScene(const Scene::Ref& scene);

	/** Blocks until the entire image is rendered.
	Not threadsafe. Only invoke this method on a single thread per RayTracer instance.
	\return An image containing an estimate of the radiance through each pixel center.
	\param pixel The pixel to trace (for debugging purposes)
	*/
	Image3::Ref render(const RenderSettings& settings, const GCamera& camera, Vector2int16 pixel = Vector2int16(ALL, ALL));
	
	void generatePhotonMap(const RenderSettings& settings);
};

#endif