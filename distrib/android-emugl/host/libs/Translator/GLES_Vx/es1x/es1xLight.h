/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 
#ifndef _ES1XLIGHT_H
#define _ES1XLIGHT_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XCOLOR_H
#	include "es1xColor.h"
#endif

#ifndef _ES1XVECTOR_H_
#	include "es1xVector.h"
#endif

typedef enum 
{
	ES1X_LIGHT_TYPE_OMNI = 0,
	ES1X_LIGHT_TYPE_SPOT,
	ES1X_LIGHT_TYPE_DIRECTIONAL,
	ES1X_NUM_LIGHT_TYPES,
} es1xLightType;

/* es1xLight */
typedef struct 
{
	es1xColor	ambient;
	es1xColor	diffuse;
	es1xColor	specular;
	es1xVec4	eyePosition;
	float		constantAttenuation;
	float		linearAttenuation;
	float		quadraticAttenuation;
	es1xVec3	spotEyeDirection;
	float		spotExponent;
	float		spotCutoff;
} es1xLight;

#ifdef __cplusplus
extern "C"
{
#endif

void es1xLight_init (es1xLight* light);

ES1X_INLINE const char* es1xLightType_toString(es1xLightType type)
{
	switch(type)
	{
		case ES1X_LIGHT_TYPE_OMNI:			return "omni";
		case ES1X_LIGHT_TYPE_SPOT:			return "spot";
		case ES1X_LIGHT_TYPE_DIRECTIONAL:	return "directional";
		default:
			ES1X_ASSERT(!"Unhandled light type");
			return "???";
	}	
}

ES1X_INLINE es1xLightType es1xLight_determineType(const es1xLight* light)
{
	if (light->eyePosition.w != 0)
		return ES1X_LIGHT_TYPE_OMNI; 
		
	if (light->spotCutoff != 180.f)
		return ES1X_LIGHT_TYPE_SPOT;
	
	return ES1X_LIGHT_TYPE_DIRECTIONAL;
}

#ifdef __cplusplus
} /* extern "C" */ 
#endif

#endif /* _ES1XLIGHT_H */
