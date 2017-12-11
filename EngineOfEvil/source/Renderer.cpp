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
#include "Game.h"

const SDL_Color clearColor		= { 128, 128, 128, SDL_ALPHA_TRANSPARENT };
const SDL_Color blackColor		= { 0, 0, 0, SDL_ALPHA_OPAQUE };
const SDL_Color greyColor_trans = { 0, 0, 0, 64 };
const SDL_Color greenColor		= { 0, 255, 0, SDL_ALPHA_OPAQUE };
const SDL_Color redColor		= { 255, 0, 0, SDL_ALPHA_OPAQUE };
const SDL_Color blueColor		= { 0, 0, 255, SDL_ALPHA_OPAQUE };
const SDL_Color pinkColor		= { 255, 0, 255, SDL_ALPHA_OPAQUE };
const SDL_Color lightBlueColor	= { 0, 255, 255, SDL_ALPHA_OPAQUE };
const SDL_Color yellowColor		= { 255, 255, 0, SDL_ALPHA_OPAQUE };
int eRenderer::globalDrawDepth	= 0;

//***************
// eRenderer::Init
// initialize the window, its rendering context, and a default font
//***************
bool eRenderer::Init() {
	window = SDL_CreateWindow(	"Evil", 
								SDL_WINDOWPOS_UNDEFINED, 
								SDL_WINDOWPOS_UNDEFINED, 
								1500, 
								800, 
								SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

	if (!window)
		return false;

	// DEBUG: TARGETTEXTURE is used to read pixel data from SDL_Textures
	internal_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

	if (!internal_renderer)
		return false;

	// enable linear anti-aliasing for the renderer context
//	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "linear" , SDL_HINT_OVERRIDE);

	if (SDL_SetRenderDrawColor(internal_renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a) == -1)	// opaque grey
		return false;

	// initialize scalableTarget for scaling textures using the camera
	// independent of any static overlay (gui/HUD)
	SDL_Rect viewArea;
	SDL_RenderGetViewport(internal_renderer, &viewArea);
	scalableTarget = SDL_CreateTexture(internal_renderer,
									   SDL_PIXELFORMAT_ARGB8888,		// DEBUG: this format may not work for all images
									   SDL_TEXTUREACCESS_TARGET,
									   viewArea.w,
									   viewArea.h);

	if (!scalableTarget)
		return false;

	// ensure the scalableTarget can alpha-blend
	SDL_SetTextureBlendMode(scalableTarget, SDL_BLENDMODE_BLEND);


	// initialize debugCameraTarget for scaling debug info using the camera
	debugCameraTarget = SDL_CreateTexture(internal_renderer,
										  SDL_PIXELFORMAT_ARGB8888,		// DEBUG: this format may not work for all images
										  SDL_TEXTUREACCESS_TARGET,
										  viewArea.w,
										  viewArea.h);

	if (!debugCameraTarget)
		return false;

	// ensure the debugCameraTarget can alhpa-blend
	SDL_SetTextureBlendMode(debugCameraTarget, SDL_BLENDMODE_BLEND);

	// initialize debugOverlayTarget for overlay debug info
	debugOverlayTarget = SDL_CreateTexture(internal_renderer,
										   SDL_PIXELFORMAT_ARGB8888,	// DEBUG: this format may not work for all images
										   SDL_TEXTUREACCESS_TARGET,
										   viewArea.w,
										   viewArea.h);

	if (!debugOverlayTarget)
		return false;

	// ensure the debugOverlayTarget can alhpa-blend
	SDL_SetTextureBlendMode(debugOverlayTarget, SDL_BLENDMODE_BLEND);



	if (TTF_Init() == -1)
		return false;

	font = TTF_OpenFont("Graphics/Fonts/Alfphabet.ttf", 24);				// FIXME: make this a file-initialized string, not hard-coded

	if (!font)
		return false;

	return true;
}

//***************
// eRenderer::Free
// close the font and destroy the window
//***************
void eRenderer::Free() const {
	if (font)
		TTF_CloseFont(font);

	TTF_Quit();

	if (internal_renderer)
		SDL_DestroyRenderer(internal_renderer);

	if (window)
		SDL_DestroyWindow(window);
}

//***************
// eRenderer::DrawOutlineText
// Immediatly draws the given string on the screen using location and color
// constText caches the text image to accelerate redraw
// dynamic moves and scales with the camera
// DEBUG: converts the input point float data to integer values
//***************
void eRenderer::DrawOutlineText(const char * text, eVec2 & point, const SDL_Color & color, bool constText, bool dynamic) {
	SDL_Texture * renderedText = NULL;
	if (constText) {
		// check if the image already exists, if not then load it and set result
		std::shared_ptr<eImage> result;
		game.GetImageManager().LoadAndGetConstantText(font, text, color, result);
		renderedText = result->Source();
	} else {
		SDL_Surface * surfaceText = TTF_RenderText_Solid(font, text, color);
		if (surfaceText == NULL)
			return;

		renderedText = SDL_CreateTextureFromSurface(internal_renderer, surfaceText);
		SDL_FreeSurface(surfaceText);
		if (renderedText == NULL)
			return;

		SDL_SetTextureBlendMode(renderedText, SDL_BLENDMODE_BLEND);
	}
	
	point -= game.GetCamera().CollisionModel().AbsBounds()[0] * dynamic;
	point.SnapInt();
	SDL_Rect dstRect = { (int)point.x, (int)point.y, 0, 0 };
	SDL_QueryTexture(renderedText, NULL, NULL, &dstRect.w, &dstRect.h);
	if (dynamic)
		SetRenderTarget(debugCameraTarget, game.GetCamera().GetZoom());
	else
		SetRenderTarget(debugOverlayTarget);

	SDL_RenderCopy(internal_renderer, renderedText, nullptr, &dstRect);
}

//***************
// eRenderer::DrawLines
// dynamic moves and scales with the camera
// draws points.size()-1 lines from front to back
// DEBUG: immediatly draws to the given render target
//***************
void eRenderer::DrawLines(const SDL_Color & color, std::vector<eVec2>  points, bool dynamic) const {
	SDL_SetRenderDrawColor(internal_renderer, color.r, color.g, color.b, color.a);

	std::vector<SDL_Point> iPoints;
	iPoints.reserve(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		eMath::CartesianToIsometric(points[i].x, points[i].y);
		points[i] -= game.GetCamera().CollisionModel().AbsBounds()[0] * dynamic;
		points[i].SnapInt();
		iPoints[i] = { (int)points[i].x, (int)points[i].y };
	}

	if (dynamic)
		SetRenderTarget(debugCameraTarget, game.GetCamera().GetZoom());
	else
		SetRenderTarget(debugOverlayTarget);
	
	SDL_RenderDrawLines(internal_renderer, iPoints.data(), iPoints.size());
	SDL_SetRenderDrawColor(internal_renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
}

//***************
// eRenderer::DrawIsometricPrism
// converts the given rect into an isomectric box
// dynamic moves and scales with the camera
// DEBUG: immediatly draws to the given render target
//***************
void eRenderer::DrawIsometricPrism(const SDL_Color & color, const eBounds3D & rect, bool dynamic) const {
	SDL_SetRenderDrawColor(internal_renderer, color.r, color.g, color.b, color.a);

	std::array<eVec3, 10> fPoints;
	rect.ToPoints(fPoints.data());
	
	// shift and insert points necessary to close the bottom and top
	for (int i = 8; i > 4; --i)
		fPoints[i] = fPoints[i - 1];
	
	fPoints[9] = fPoints[4];
	fPoints[4] = fPoints[0];

	// convert to isometric rhombus
	// and translate with camera
	std::array<SDL_Point, 10> iPoints;
	for (size_t i = 0; i < fPoints.size(); ++i) {
		fPoints[i].x -= fPoints[i].z;
		fPoints[i].y -= fPoints[i].z;
		eMath::CartesianToIsometric(fPoints[i].x, fPoints[i].y);
		fPoints[i] -= (eVec3)game.GetCamera().CollisionModel().AbsBounds()[0] * dynamic;
		fPoints[i].SnapInt();
		iPoints[i] = { (int)fPoints[i].x, (int)fPoints[i].y };
	}

	if (dynamic)
		SetRenderTarget(debugCameraTarget, game.GetCamera().GetZoom());
	else
		SetRenderTarget(debugOverlayTarget);

	SDL_RenderDrawLines(internal_renderer, iPoints.data(), iPoints.size());

	std::array<SDL_Point, 2> verticals;
	verticals[0] = iPoints[6];
	verticals[1] = iPoints[1];
	SDL_RenderDrawLines(internal_renderer, verticals.data(), verticals.size());

	verticals[0] = iPoints[7];
	verticals[1] = iPoints[2];
	SDL_RenderDrawLines(internal_renderer, verticals.data(), verticals.size());

	verticals[0] = iPoints[8];
	verticals[1] = iPoints[3];
	SDL_RenderDrawLines(internal_renderer, verticals.data(), verticals.size());
}

//***************
// eRenderer::DrawIsometricRect
// converts the given rect into an isomectric box
// dynamic moves and scales with the camera
// DEBUG: immediatly draws to the given render target
//***************
void eRenderer::DrawIsometricRect(const SDL_Color & color, const eBounds & rect, bool dynamic) const {
	SDL_SetRenderDrawColor(internal_renderer, color.r, color.g, color.b, color.a);

	std::array<eVec2, 5> fPoints;
	rect.ToPoints(fPoints.data());
	fPoints[4] = fPoints[0];

	// convert to isometric rhombus
	// and translate with camera
	std::array<SDL_Point, 5> iPoints;
	for (size_t i = 0; i < fPoints.size(); ++i) {
		eMath::CartesianToIsometric(fPoints[i].x, fPoints[i].y);
		fPoints[i] -= game.GetCamera().CollisionModel().AbsBounds()[0] * dynamic;
		fPoints[i].SnapInt();
		iPoints[i] = { (int)fPoints[i].x, (int)fPoints[i].y };
	}

	if (dynamic)
		SetRenderTarget(debugCameraTarget, game.GetCamera().GetZoom());
	else
		SetRenderTarget(debugOverlayTarget);

	SDL_RenderDrawLines(internal_renderer, iPoints.data(), iPoints.size());
}

//***************
// eRenderer::DrawCartesianRect
// fill draws the rect solid
// dynamic moves and scales with the camera
// DEBUG: immediatly draws to the given render target
//***************
void eRenderer::DrawCartesianRect(const SDL_Color & color, const eBounds & rect, bool fill, bool dynamic) const {
	SDL_SetRenderDrawColor(internal_renderer, color.r, color.g, color.b, color.a);

	auto & cameraMins = game.GetCamera().CollisionModel().AbsBounds()[0];
	SDL_Rect drawRect = {	eMath::NearestInt(rect[0].x - cameraMins.x * dynamic), 
							eMath::NearestInt(rect[0].y - cameraMins.y * dynamic), 
							eMath::NearestInt(rect.Width()), 
							eMath::NearestInt(rect.Height()) };

	if (dynamic)
		SetRenderTarget(debugCameraTarget, game.GetCamera().GetZoom());
	else
		SetRenderTarget(debugOverlayTarget);

	fill ? SDL_RenderFillRect(internal_renderer, &drawRect)
		: SDL_RenderDrawRect(internal_renderer, &drawRect);
}

//***************
// eRenderer::DrawImage
// RENDERTYPE_DYNAMIC adjusts image origin by camera position ("transform")
// RENDERTYPE_STATIC does not
// FIXME: rendertype doesn't have the same meaning here as in AddTo...RenderPool,
// but it should (or shouldn't be a parameter)
//***************
void eRenderer::DrawImage(eRenderImage * renderImage, eRenderType_t renderType) const {
	eVec2 drawPoint = renderImage->origin;
	if (renderType == RENDERTYPE_DYNAMIC) {
		const auto & cameraAdjustment = game.GetCamera().CollisionModel().AbsBounds()[0];
		drawPoint -= cameraAdjustment;
	}
	drawPoint.SnapInt();
	renderImage->dstRect = { (int)drawPoint.x, (int)drawPoint.y, renderImage->srcRect->w, renderImage->srcRect->h };
	SDL_RenderCopy(internal_renderer, renderImage->image->Source(), renderImage->srcRect, &renderImage->dstRect);
}

//***************
// eRenderer::AddToCameraRenderPool
//***************
void eRenderer::AddToCameraRenderPool(eRenderImage * renderImage) {
	const auto & gameTime = game.GetGameTime();
	if (renderImage->lastDrawTime == gameTime)
		return;
	renderImage->lastDrawTime = gameTime;
	auto targetPool = (renderImage->Owner()->IsStatic() ? &cameraPool : &cameraPoolInserts);
	targetPool->emplace_back(renderImage);
}

//***************
// eRenderer::AddToOverlayRenderPool
//***************
void eRenderer::AddToOverlayRenderPool(eRenderImage * renderImage) {
	const auto & gameTime = game.GetGameTime();
	if (renderImage->lastDrawTime == gameTime)
		return;
	renderImage->lastDrawTime = gameTime;
	auto targetPool = (renderImage->Owner()->IsStatic() ? &overlayPool : &overlayPoolInserts);
	targetPool->emplace_back(renderImage);
}

//***************
// eRenderer::TopologicalDrawDepthSort
// assigns draw order priority to the given eRenderImage(s)
// based on their positions relative to the camera
// DEBUG: this is best used on either an entire eRenderer::staticPool/eRenderer::dynamicPool for a frame
// or ONCE for all static geometry in game at startup, followed by adjusting the eRenderImage::priority of dynamic geometry separately
// (starting, for example, with calling this with those items to establish a "localDrawDepth" order amongst them)
//***************
void eRenderer::TopologicalDrawDepthSort(const std::vector<eRenderImage *> & renderImagePool) {
	for (auto & self : renderImagePool) {
		auto & selfClip = self->worldClip;

		for (auto & other : renderImagePool) {
			auto & otherClip = other->worldClip;

			if (other != self && 
				eCollision::AABBAABBTest(selfClip, otherClip) && 
				eCollision::IsAABB3DInIsometricFront(self->renderBlock, other->renderBlock))
				self->allBehind.emplace_back(other);
		}
		self->visited = false;
	}

	globalDrawDepth = 0;
	for (auto & renderImage : renderImagePool)
		VisitTopologicalNode(renderImage);
}

//***************
// eRenderer::VisitTopologicalNode
//***************
void eRenderer::VisitTopologicalNode(eRenderImage * renderImage) {
	if (!renderImage->visited) {
		renderImage->visited = true;
		while (!renderImage->allBehind.empty()) {
			VisitTopologicalNode(renderImage->allBehind.back());
			renderImage->allBehind.pop_back();
		}
		renderImage->priority = (float)globalDrawDepth++;
		renderImage->allBehind.clear();
	}
}

//***************
// eRenderer::Flush
//***************
void eRenderer::Flush() {
	FlushCameraPool();
	SetRenderTarget(scalableTarget);
	SDL_RenderCopy(internal_renderer, debugCameraTarget, NULL, NULL);

	// transfer camera (and its debug) info to the main render texture
	SetRenderTarget(defaultTarget);
	SDL_RenderCopy(internal_renderer, scalableTarget, NULL, NULL);

	// draw all overlay (HUD/UI/Screen-Space) info now
	FlushOverlayPool();
	SetRenderTarget(defaultTarget);
	SDL_RenderCopy(internal_renderer, debugOverlayTarget, NULL, NULL);
}

//***************
// eRenderer::FlushCameraPool
// DEBUG: this unstable quicksort may put renderImages at random draw orders if they have equal priority
//***************
void eRenderer::FlushCameraPool() {
/*
	auto targetPool = RENDERTYPE_CAMERA ? &cameraPool : &screenPool);
	auto targetPoolInserts = RENDERTYPE_CAMERA ? &cameraPoolInserts : &screenPoolInserts);
	// then check for RENDERTYPE_CAMERA again to set/reset the rendertarget
*/



	// sort the dynamicPool for the scalableTarget
	QuickSort(	cameraPool.data(),
				cameraPool.size(),
				[](auto && a, auto && b) {
					if (a->priority < b->priority) return -1;
					else if (a->priority > b->priority) return 1;
					return 0;
			});

// FREEHILL BEGIN 3d topological sort
	TopologicalDrawDepthSort(cameraPoolInserts);	// assign a "localDrawDepth" priority amongst the cameraPoolInserts
	float newPriorityMin = 0.0f;
	for (auto & imageToInsert : cameraPoolInserts) {
		float newPriorityMax = 0.0f;

		// minize time sent re-sorting static renderImages and just insert the dynamic ones
		auto & iter = cameraPool.begin();
		for (/*iter*/; iter != cameraPool.end() ; ++iter) {
			if (eCollision::AABBAABBTest(imageToInsert->worldClip, (*iter)->worldClip)) {	
				if (eCollision::IsAABB3DInIsometricFront(imageToInsert->renderBlock, (*iter)->renderBlock)) {
					newPriorityMin = (*iter)->priority;
				} else {
					newPriorityMax = (*iter)->priority;
					break;
				}
			}
		}
		float newPriority = (newPriorityMin + newPriorityMax) * 0.5f;
		imageToInsert->priority = newPriority;		// needed in the event this renderImage goes straight to dynamicPool next frame
		cameraPool.emplace(iter, imageToInsert);
		newPriorityMin = newPriority;
		// FIXME/BUG(~): ensure iter is properly positioned for the next imageToInsert search works
		// because iter may be pointing to the newly inserted item (or one in front of it) in dynamicPool
		// or utterly somewhere else if dynamicPool resized (which is unlikely given that it has defaultRenderCapacity[1024] reserved)
	}
// FREEHILL END 3d topological sort

	// set the render target, and scale according to camera zoom
	SetRenderTarget(scalableTarget, game.GetCamera().GetZoom());

	// draw to the scalableTarget
	for (auto && renderImage : cameraPool)
		DrawImage(renderImage, RENDERTYPE_DYNAMIC);

	cameraPool.clear();
	cameraPoolInserts.clear();
}

//***************
// eRenderer::FlushOverlayPool
// DEBUG: this unstable quicksort may put renderImages at random draw orders if they have equal priority
//***************
void eRenderer::FlushOverlayPool() {
	// sort the staticPool for the default render target
	QuickSort(	overlayPool.data(),
				overlayPool.size(), 
				[](auto && a, auto && b) { 
					if (a->priority < b->priority) return -1;
					else if (a->priority > b->priority) return 1;
					return 0; 
			});

	// set the render target, and scale to 1.0f
	SetRenderTarget(defaultTarget);

	for (auto && renderImage : overlayPool)
		DrawImage(renderImage, RENDERTYPE_STATIC);

	overlayPool.clear();
	overlayPoolInserts.clear();
}

