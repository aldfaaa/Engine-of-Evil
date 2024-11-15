/*
===========================================================================

Engine of Evil GPL Source Code
Copyright (C) 2016-2017 Thomas Matthew Freehill 

This file is part of the Engine of Evil GPL game engine source code. 

The Engine of Evil (EOE) Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

EOE Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with EOE Source Code.  If not, see <http://www.gnu.org/licenses/>.


If you have questions concerning this license, you may contact Thomas Freehill at tom.freehill26@gmail.com

===========================================================================
*/
#include "Dictionary.h"

//********************
// eDictionary::GetVec2
//********************
eVec2 eDictionary::GetVec2( const char *key, const char *defaultString ) const {
	eVec2			result;
	const char *	resultString;
	
	if (!defaultString)
		defaultString = "0 0";

	resultString = GetString(key, defaultString);
	sscanf(resultString, "%f %f", &result.x, &result.y);
	return result;
}

//********************
// eDictionary::GetVec3
//********************
eVec3 eDictionary::GetVec3( const char *key, const char *defaultString) const {
	eVec3		 result;
	const char * resultString;
	
	if (!defaultString)
		defaultString = "0 0 0";

	resultString = GetString(key, defaultString);
	sscanf(resultString, "%f %f %f", &result.x, &result.y, &result.z);
	return result;
}

//********************
// eDictionary::GetVec4
//********************
eQuat eDictionary::GetVec4( const char *key, const char *defaultString) const {
	eQuat		 result;
	const char * resultString;
	
	if (!defaultString)
		defaultString = "0 0 0 0";

	resultString = GetString( key, defaultString );
	sscanf(resultString, "%f %f %f %f", &result.x, &result.y, &result.z, &result.w);
	return result;
}