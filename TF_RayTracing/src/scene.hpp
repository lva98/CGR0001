#ifndef __RT_SCENE__
#define __RT_SCENE__

#include "objects.hpp"

void InitDefaultScene()
{
	//Planes.push_back({{{ 0, 0,-1}}, -90});
	Planes.push_back({{{ 0, 1, 0}}, -30});
	Planes.push_back({{{ 0,-1, 0}}, -30});
	Planes.push_back({{{ 1, 0, 0}}, -30});
	Planes.push_back({{{ 0, 0, 1}}, -90});
	Planes.push_back({{{-1, 0, 0}}, -30});

	Spheres.push_back({{{  0.0,  0.0,  0.0}},  7.0});
	Spheres.push_back({{{ 19.4,-19.4,  0.0}},  2.1});
	Spheres.push_back({{{-19.4, 19.4,  0.0}},  2.1});
	Spheres.push_back({{{ 13.1,  5.1,  0.0}},  1.1});
	Spheres.push_back({{{ -5.1,-13.1,  0.0}},  1.1});
	Spheres.push_back({{{-30.0, 30.0, 15.0}}, 11.0});
	Spheres.push_back({{{ 15.0,-30.0, 30.0}},  6.0});
	Spheres.push_back({{{ 30.0, 15.0,-30.0}},  6.0});
	
	Lights.push_back({{{-28,-14,  3}}, {{ .40, .51, .90}}});
	Lights.push_back({{{-29,-29,-29}}, {{ .95, .10, .10}}});
	Lights.push_back({{{ 14, 29,-14}}, {{ .80, .80, .80}}});
	Lights.push_back({{{ 29, 29, 29}}, {{1.00,1.00,1.00}}});
	Lights.push_back({{{ 28,  0, 29}}, {{ .50, .60, .10}}});
}

#endif
