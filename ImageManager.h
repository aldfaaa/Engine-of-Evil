#ifndef EVIL_IMAGE_MANAGER_H
#define EVIL_IMAGE_MANAGER_H

#include "Definitions.h"
#include "Image.h"
#include "HashIndex.h"
#include "Renderer.h"

//**********************************
//			eImageManager
// Handles all surface allocation and feeing
// DEBUG: --no other object/system should allocate/free images--
// DEBUG: only modify surface data as it is being BLITTED, 
// not before, ie mask/modify it enroute to the screen
// not in memory otherwise it will have game-wide
// consequences when drawing anything that uses
// that same resource image
//**********************************
class eImageManager {
public:

					eImageManager();

	eImage *		GetImage(const char * name, int * colorKey);
	void			Free();

private:

	eImage			images[MAX_IMAGES];
	eHashIndex		imageHash;
	int				numImages;
};

//***************
// eImageManager::eImageManager
//***************
inline eImageManager::eImageManager() 
	: numImages(0) {
}

#endif /* EVIL_IMAGE_MANAGER_H */

