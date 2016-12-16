#include "Camera.h"
#include "Game.h"

//***************
// eCamera::Init
// TODO: allow the localBounds to be resized as the window resizes or goes fullscreen
//***************
void eCamera::Init() {
	eVec2 screenCorner;

	// starting view of map
	screenCorner = eVec2((float)game.GetRenderer().Width(), (float)game.GetRenderer().Height());
	localBounds = eBounds(-screenCorner / 2.0f, screenCorner / 2.0f);	// variable rectangle with (0, 0) at its center)
	SetOrigin(screenCorner / 2.0f);
}

//***************
// eCamera::Think
//***************
void eCamera::Think() {
	eInput * input;
	eVec2 correction;
	float x, y;

	static const int maxX = game.GetMap().TileMap().Width() > (int)localBounds.Width() ? game.GetMap().TileMap().Width() : (int)localBounds.Width();
	static const int maxY = game.GetMap().TileMap().Height() > (int)localBounds.Height() ? game.GetMap().TileMap().Height() : (int)localBounds.Height();

	input = &game.GetInput();
	if (input->KeyHeld(SDL_SCANCODE_SPACE)) {
		SetOrigin(game.GetEntity(0)->Origin());
	} else {
		x = speed * (float)(input->KeyHeld(SDL_SCANCODE_D) - input->KeyHeld(SDL_SCANCODE_A));
		y = speed * (float)(input->KeyHeld(SDL_SCANCODE_S) - input->KeyHeld(SDL_SCANCODE_W));
		velocity.Set(x, y);
		UpdateOrigin();
	}

	// collision response with map edge
	// TODO(?): move this to a collision detection/handling class
	correction = vec2_zero;
	if (absBounds[0].x < 0)
		correction.x = -absBounds[0].x;
	else if (absBounds[1].x > maxX)
		correction.x = maxX - absBounds[1].x;

	if (absBounds[0].y < 0)
		correction.y = -absBounds[0].y;
	else if (absBounds[1].y > maxY)
		correction.y = maxY - absBounds[1].y;

	if (correction != vec2_zero)
		SetOrigin(origin + correction);
}
