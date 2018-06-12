#ifndef __RT_HEADERS__
#define __RT_HEADERS__

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

// Raytracing is mathematics-heavy. Declare mathematical datatypes
inline double dmin(double a,double b) { return a<b ? a : b; }
struct XYZ
{
	double d[3];
	// Declare operators pertinent to vectors in general:
	inline void Set(double a,double b,double c) { d[0]=a; d[1]=b; d[2]=c;}
	#define do_op(o) \
		inline void operator o##= (const XYZ& b) { for(unsigned n=0; n<3; ++n) d[n] o##= b.d[n]; } \
		inline void operator o##= (double b)	 { for(unsigned n=0; n<3; ++n) d[n] o##= b; } \
		XYZ operator o (const XYZ& b) const	  { XYZ tmp(*this); tmp o##= b; return tmp; } \
		XYZ operator o (double b)	 const	  { XYZ tmp(*this); tmp o##= b; return tmp; }
	do_op(*)
	do_op(+)
	do_op(-)
	#undef do_op
	XYZ operator- () const { XYZ tmp = { { -d[0], -d[1], -d[2] } }; return tmp; }
	XYZ Pow(double b) const
		{ XYZ tmp = {{ pow(d[0],b), pow(d[1],b), pow(d[2],b) }}; return tmp; }
	// Operators pertinent to geometrical vectors:
	inline double Dot(const XYZ& b) const
		{ return d[0]*b.d[0] + d[1]*b.d[1] + d[2]*b.d[2]; }
	inline double Squared() const	 { return Dot(*this); }
	inline double Len()	 const	 { return sqrt(Squared()); }
	inline void Normalize()		   { *this *= 1.0 / Len(); }
	void MirrorAround(const XYZ& axis)
	{
		XYZ N = axis; N.Normalize();
		double v = Dot(N);
		*this = N * (v+v) - *this;
	}
	// Operators pertinent to colour vectors (RGB):
	inline double Luma() const { return d[0]*0.299 + d[1]*0.587 + d[2]*0.114; }
	void Clamp()
	{
		for(unsigned n=0; n<3; ++n)
		{
			if(d[n] < 0.0) d[n] = 0.0;
			else if(d[n] > 1.0) d[n] = 1.0;
		}
	}
	void ClampWithDesaturation()
	{
		// If the color represented by this triplet
		// is too bright or too dim, decrease the
		// saturation as much as required, while keeping
		// the luma unmodified.
		double l = Luma(), sat = 1.0;
		if(l > 1.0) { d[0] = d[1] = d[2] = 1.0; return; }
		if(l < 0.0) { d[0] = d[1] = d[2] = 0.0; return; }
		// If any component is over the bounds, calculate how
		// much the saturation must be reduced to achieve an
		// in-bounds value. Since the luma was verified to be
		// in 0..1 range, a maximum reduction of saturationto
		// 0% will always produce an in-bounds value, but
		// usually such a drastic reduction is not necessary.
		// Because we're only doing relative modifications,
		// we don't need to determine the original saturation
		// level of the pixel.
		for(int n=0; n<3; ++n)
			if(d[n] > 1.0) sat = dmin(sat, (l-1.0) / (l-d[n]));
			else if(d[n] < 0.0) sat = dmin(sat, l  / (l-d[n]));
		if(sat != 1.0)
			 { *this = (*this - l) * sat + l; Clamp(); }
	}
};
struct Matrix
{
	XYZ m[3];
	void InitRotate(const XYZ& angle)
	{
		double Cx = cos(angle.d[0]), Cy = cos(angle.d[1]), Cz = cos(angle.d[2]);
		double Sx = sin(angle.d[0]), Sy = sin(angle.d[1]), Sz = sin(angle.d[2]);
		double sxsz = Sx*Sz, cxsz = Cx*Sz;
		double cxcz = Cx*Cz, sxcz = Sx*Cz;
		Matrix result = {{ {{ Cy*Cz, Cy*Sz, -Sy }},
						   {{ sxcz*Sy - cxsz, sxsz*Sy + cxcz, Sx*Cy }},
						   {{ cxcz*Sy + sxsz, cxsz*Sy - sxcz, Cx*Cy }} }};
		*this = result;
	}
	void Transform(XYZ& vec)
	{
		vec.Set( m[0].Dot(vec), m[1].Dot(vec), m[2].Dot(vec) );
	}
};


// Rendering procedure

// Declarations for scene description
// Walls are planes. Planes have a
// normal vector and a distance.
const struct Plane
{ XYZ normal; double offset; }
Planes[] =
{
// Declare six planes, each looks 
// towards origo and is 30 units away.
	{ {{0,0,-1}}, -90 },
	{ {{0, 1,0}}, -30 },
	{ {{0,-1,0}}, -30 },
	{ {{ 1,0,0}}, -30 },
	{ {{0, 0,1}}, -90 },
	{ {{-1,0,0}}, -30 }
};
const struct Sphere
{ XYZ center; double radius; }
Spheres[] =
{
// Declare a few spheres.
	{ {{0,0,0}}, 7 },
	{ {{19.4, -19.4, 0}}, 2.1 },
	{ {{-19.4, 19.4, 0}}, 2.1 },
	{ {{13.1, 5.1, 0}}, 1.1 },
	{ {{ -5.1, -13.1, 0}}, 1.1 },
	{ {{-30,30,15}}, 11},
	{ {{15,-30,30}}, 6},
	{ {{30,15,-30}}, 6}
};
// Declare lightsources. A lightsource
// has a location and a RGB color.
const struct LightSource
{ XYZ where, colour; }
Lights[] =
{
	{ {{-28,-14, 3}}, {{.4, .51, .9}} },
	{ {{-29,-29,-29}}, {{.95, .1, .1}} },
	{ {{ 14, 29,-14}}, {{.8, .8, .8}} },
	{ {{ 29, 29, 29}}, {{1,1,1}} },
	{ {{ 28,  0, 29}}, {{.5, .6,  .1}} }
};
#define NElems(x) sizeof(x)/sizeof(*x)
const unsigned
	NumPlanes = NElems(Planes),
	NumSpheres = NElems(Spheres),
	NumLights = NElems(Lights);
unsigned MAXTRACE = 2; // Maximum trace level

/* Actual raytracing! */

/**************/
// Determine whether an object is
// in direct eyesight on the given
// line, and determine exactly which
// point of the object is seen.
int RayFindObstacle
	(const XYZ& eye, const XYZ& dir,
	 double& HitDist, int& HitIndex,
	 XYZ& HitLoc, XYZ& HitNormal)
{
	// Try intersecting the ray with
	// each object and see which one
	// produces the closest hit.
	int HitType = -1;
	for(unsigned i=0; i<NumSpheres; ++i)
	{
		XYZ V (eye - Spheres[i].center);
		double r = Spheres[i].radius,
			DV = dir.Dot(V),
			D2 = dir.Squared(),
			SQ = DV*DV
			   - D2*(V.Squared() - r*r);
		// Does the ray coincide
		// with the sphere?
		if(SQ < 1e-6) continue;
		// Determine where exactly
		double SQt = sqrt(SQ),
			Dist = dmin(-DV-SQt,
						-DV+SQt) / D2;
		if(Dist<1e-6 || Dist >= HitDist)
			continue;
		HitType = 1; HitIndex = i;
		HitDist = Dist;
		HitLoc = eye + (dir * HitDist);
		HitNormal =
			(HitLoc - Spheres[i].center)
				* (1/r);
	}
	for(unsigned i=0; i<NumPlanes; ++i)
	{
		double DV = -Planes[i].normal.Dot(dir);
		if(DV > -1e-6) continue;
		double D2 =
			Planes[i].normal.Dot(eye),
			Dist = (D2+Planes[i].offset) / DV;
		if(Dist<1e-6 || Dist>=HitDist)
			continue;
		HitType = 0; HitIndex = i;
		HitDist = Dist;
		HitLoc = eye + (dir * HitDist);
		HitNormal = -Planes[i].normal;
	}
	return HitType;
}

const unsigned NumArealightVectors = 20;
XYZ ArealightVectors[NumArealightVectors];
void InitArealightVectors()
{
	// To smooth out shadows cast by lightsources,
	// I plan to approximate the lightsources with
	// a _cloud_ of lightsources around the point
	for(unsigned i=0; i<NumArealightVectors; ++i)
		for(unsigned n=0; n<3; ++n)
			ArealightVectors[i].d[n] =
				2.0 * (rand() / double(RAND_MAX) - 0.5);
}

// Shoot a camera-ray from the specified location
// to specified location, and determine the RGB
// color of the perception corresponding to that
// location.
void RayTrace(XYZ& resultcolor, const XYZ& eye, const XYZ& dir, int k)
{
	double HitDist = 1e6;
	XYZ HitLoc, HitNormal;
	int HitIndex, HitType;
	HitType = RayFindObstacle(eye,dir, HitDist,HitIndex, HitLoc,HitNormal);
	if(HitType != -1)
	{
		// Found an obstacle. Next, find out how it is illuminated.
		// Shoot a ray to each lightsource, and determine if there
		// is an obstacle behind it. This is called "diffuse light".
		// To smooth out the infinitely sharp shadows caused by
		// infinitely small point-lightsources, assume the lightsource
		// is actually a cloud of small lightsources around its center.
		XYZ DiffuseLight = {{0,0,0}}, SpecularLight = {{0,0,0}};
		XYZ Pigment = {{1, 0.98, 0.94}}; // default pigment
		for(unsigned i=0; i<NumLights; ++i)
			for(unsigned j=0; j<NumArealightVectors; ++j)
			{
				XYZ V((Lights[i].where + ArealightVectors[j]) - HitLoc);
				double LightDist = V.Len();
				V.Normalize();
				double DiffuseEffect = HitNormal.Dot(V) / (double)NumArealightVectors;
				double Attenuation = (1 + pow(LightDist / 34.0, 2.0));
				DiffuseEffect /= Attenuation;
				if(DiffuseEffect > 1e-3)
				{
					double ShadowDist = LightDist - 1e-4;
					XYZ a,b;
					int q, t = RayFindObstacle(HitLoc + V*1e-4, V, ShadowDist, q, a, b);
					if(t == -1) // No obstacle occluding the light?
						DiffuseLight += Lights[i].colour * DiffuseEffect;
				}
			}
		if(k > 1)
		{
			// Add specular light/reflection, unless recursion depth is at max
			XYZ V(-dir); V.MirrorAround(HitNormal);
			RayTrace(SpecularLight, HitLoc + V*1e-4, V, k-1);
		}

		// AQUI COMEÇA CÁLCULO DE LUMINOSIDADE BASEADO EM MATERIAIS

		switch(HitType)
		{
			case 0: // plane
				DiffuseLight *= 0.9;
				SpecularLight *= 0.5;
				// Color the different walls differently
				switch(HitIndex % 4)
				{
					case 0: Pigment.Set(0.9, 0.7, 0.6); break;
					case 1: Pigment.Set(0.6, 0.7, 0.7); break;
					case 2: Pigment.Set(0.5, 0.8, 0.3); break;
					case 3: Pigment.Set(0.1, 0.1, 0.1); break;
				}
				break;
			case 1: // sphere
				DiffuseLight  *= 1.0;
				SpecularLight  *= 0.74;
		}
		resultcolor = (DiffuseLight + SpecularLight) * Pigment;

		// AQUI TERMINA CÁLCULO DE LUMINOSIDADE BASEADO EM MATERIAIS
	}
}

XYZ camangle	  = {{0,0,0}};
XYZ camangledelta = {{-.005, -.011, -.017}};
XYZ camlook       = {{0,0,0}};
XYZ camlookdelta  = {{-.001, .005, .004}};
XYZ campos        = {{ 0.0, -3.0, 16.0}};

//double zoom = 46.0, zoomdelta = 0.99;
double zoom = 3.0, zoomdelta = 0.99;
double contrast = 6.2, contrast_offset = -0.12;

/* MAIN PROGRAM */
void render(unsigned W, unsigned H, sf::Uint8 *pixels)
{
	double AR = W/double(H);

	InitArealightVectors();

	// Put camera between the central sphere and the walls
	// Rotate it around the center
	Matrix camrotatematrix, camlookmatrix;
	camrotatematrix.InitRotate(camangle);
	camrotatematrix.Transform(campos);
	camlookmatrix.InitRotate(camlook);

	// Determine the contrast ratio for this frame's pixels
	double thisframe_min = 100;
	double thisframe_max = -100;

  #pragma omp parallel for collapse(2)
	for(unsigned y=0; y<H; ++y)
		for(unsigned x=0; x<W; ++x)
		{
			XYZ camray = {{ x / double(W) - 0.5,
							y / double(H) - 0.5,
							zoom }};
			camray.d[0] *= AR; // Aspect ratio correction
			camray.Normalize();
			camlookmatrix.Transform(camray);
			XYZ campix;
			RayTrace(campix, campos, camray, MAXTRACE);
			campix *= 0.5;
		  #pragma omp critical
		  {
			// Update frame luminosity info for automatic contrast adjuster
			double lum = campix.Luma();
			#pragma omp flush(thisframe_min,thisframe_max)
			if(lum < thisframe_min) thisframe_min = lum;
			if(lum > thisframe_max) thisframe_max = lum;
			#pragma omp flush(thisframe_min,thisframe_max)
		  }
			// Exaggerate the colors to bring contrast better forth
			campix = (campix + contrast_offset) * contrast;
			// Clamp, and compensate for display gamma (for dithering)
			campix.ClampWithDesaturation();
			//XYZ campixG = campix.Pow(Gamma);
			//XYZ qtryG = campixG;

			// TODO Draw pixel
			{
				//printf("Cor: %f %f %f\n", campix.d[0], campix.d[1], campix.d[2]);
					
				pixels[((y*W) + x)*4 + 0] = unsigned(campix.d[0]*255); // Red
				pixels[((y*W) + x)*4 + 1] = unsigned(campix.d[1]*255); // Green
				pixels[((y*W) + x)*4 + 2] = unsigned(campix.d[2]*255); // Blue
				pixels[((y*W) + x)*4 + 3] = 255; // Alpha
				
				/*
				sf::Color px((campix.d[0] * 255),
							 (campix.d[1] * 255),
							 (campix.d[2] * 255));
				image.setPixel(x, y, px);
				*/
			}
		}

	if (toFile)
	{
		image.create(W, H, pixels);
		image.saveToFile(outfile);
		break;
	}
	else
	{
		// Atualizar textura
		texture.update(pixels);
		// Apresentar resultado
		window.draw(sprite);
		window.display();
	}

	printf("Terminado frame %4d\n", frameno);
		
	// Tweak coordinates / camera parameters for the next frame
	double much = 1.0;

	// In the beginning, do some camera action (play with zoom)
	if(zoom <= 1.1)
		zoom = 1.1;
	else
	{
		if(zoom > 40) { if(zoomdelta > 0.95) zoomdelta -= 0.001; }
		else if(zoom < 3) { if(zoomdelta < 0.99) zoomdelta += 0.001; }
		zoom *= zoomdelta;
		much = 1.1 / pow(zoom/1.1, 3);
	}

	// Update the rotation angle
	//camlook  += camlookdelta * much;
	//camangle += camangledelta * much;

	// Dynamically readjust the contrast based on the contents
	// of the last frame
	double middle = (thisframe_min + thisframe_max) * 0.5;
	double span   = (thisframe_max - thisframe_min);
	thisframe_min = middle - span*0.60; // Avoid dark tones
	thisframe_max = middle + span*0.37; // Emphasize bright tones
	double new_contrast_offset = -thisframe_min;
	double new_contrast		= 1 / (thisframe_max - thisframe_min);
	// Avoid too abrupt changes, though
	double l = 0.85;
	if(frameno == 0) l = 0.7;
	contrast_offset = (contrast_offset*l + new_contrast_offset*(1.0-l));
	contrast		= (contrast*l + new_contrast*(1.0-l));
}
/* There. The previous part was just basic mathematics. Vector and matrix mathematics. */

#endif