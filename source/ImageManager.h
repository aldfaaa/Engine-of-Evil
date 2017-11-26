#ifndef EVIL_TEXTURE_MANAGER_H
#define EVIL_TEXTURE_MANAGER_H

#include "HashIndex.h"
#include "Renderer.h"

//#undef LoadImage		// FIXME: weird #define in WinUser.h brought in from #include <Windows.h> in main.cpp
						// SOLUTION: use SDLmain.dll and avoid WinMain [or get rid of main altogether to compile as a library in EngineOfEvil-Core]

//**********************************
//			eImageManager
// Handles all texture allocation and freeing
// DEBUG: --no other object/system should allocate/free textures--
//**********************************
class eImageManager : public eClass {
public:

	bool			Init();
	bool			BatchLoadImages(const char * imageBatchFile);
	bool			GetImage(const char * filename, std::shared_ptr<eImage> & result);
	bool			GetImage(int imageID, std::shared_ptr<eImage> & result);
	bool			LoadImage(const char * filename, SDL_TextureAccess accessType, std::shared_ptr<eImage> & result);
	bool			LoadConstantText(TTF_Font * font, const char * text, const SDL_Color & color, std::shared_ptr<eImage> & result);
	bool			LoadImageSubframes(const char * filename);
	bool			BatchLoadSubframes(const char * subframeBatchFile);
	int				GetNumImages() const;
	void			Clear();

	virtual int		GetClassType() const override { return CLASS_IMAGE_MANAGER; }

private:

	std::vector<std::shared_ptr<eImage>>		imageList;			// dynamically allocated image resources
	eHashIndex									imageFilenameHash;	// quick access to imageList
};

//***************
// eImageManager::GetNumImages
//***************
inline int eImageManager::GetNumImages() const {
	return imageList.size();
}

#endif /* EVIL_TEXTURE_MANAGER_H */
