#include "Entity.h"
#include "Game.h"

//***************
// eEntity::Spawn
//***************
bool eEntity::Spawn() {

	// TODO: pass in some spawn arguments for the animation definitions file
	// TODO: set the initial (no input/movement) default animation
	std::shared_ptr<eImage> spriteImage;
	if (!game.GetImageManager().LoadImage("graphics/hero.bmp", SDL_TEXTUREACCESS_STATIC, spriteImage))
		return false;

	// TODO: call sprite.Init("imageTilerBatchLoadFilename.sprite");
	// where all the imageTilers have ALREADY been eImageTilerManager::BatchLoad(filename)
	// and the sprite just eImageTilerManager.GetTiler a bunch of times according to the sprite's file
	// AND potentially loads various control varibles from the sprite's file as well (maybe)
	// TODO: after the sprite has been fully loaded WRITE a spriteController class
	// that dictates transitions between tilers and intra-tiler-sequences (similar to Button.h)
	sprite.SetImage(spriteImage);		// TODO: change this to a sprite.Init(...) maybe and return false if it fails

	localBounds.ExpandSelf(8);			// FIXME: 16 x 16 square with (0, 0) at its center, 
										// this is the current collision box
										// but its also used to position the sprite, move away from this methodology
										// becuase a hit box may be smaller than the sprite... give the sprite an origin? YES.

	SetOrigin(eVec2(8.0f, 8.0f));		// TODO: call a GetSpawnPoint() to use a list of procedurally/file-defined spawn points
	return true;
}

//***************
// eEntity::Draw
// draw the sprite at its current animation frame
//***************
void eEntity::Draw() {
	eVec2 drawPoint = absBounds[0];
	eMath::CartesianToIsometric(drawPoint.x, drawPoint.y);
	drawPoint -= game.GetCamera().absBounds[0];
	drawPoint.SnapInt();
	
	const SDL_Rect & srcRect = sprite.GetFrameHack();
	SDL_Rect dstRect = { (int)drawPoint.x, (int)drawPoint.y, srcRect.w, srcRect.h };
	game.GetRenderer().AddToRenderPool(renderImage_t{ sprite.GetImage(), &srcRect, dstRect, 1 }, RENDERTYPE_DYNAMIC);	// DEBUG: test layer == 1	
}

