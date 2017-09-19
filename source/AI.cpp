#include "AI.h"
#include "Game.h"

//***************
// eAI::Spawn
// TODO: make this spawn independent of eEntity::Spawn
//***************
bool eAI::Spawn() {

	if (!eEntity::Spawn())
		return false;

	collisionRadius = collisionModel.LocalBounds().Radius();
	StopMoving();

	// knownMap dimensions, based in initialized tileMap dimensions
	knownMap.SetCellWidth(game.GetMap().TileMap().CellWidth());
	knownMap.SetCellHeight(game.GetMap().TileMap().CellHeight());
	knownMap.ClearAllCells();

	currentTile		= &knownMap.Index(collisionModel.Origin());
	previousTile	= currentTile;
	lastTrailTile	= nullptr;
	currentWaypoint = nullptr;
	wallSide		= nullptr;
	pathingState	= PATHTYPE_NONE;
	moveState		= MOVETYPE_NONE;
	return true;
}

//***************
// eAI::Think
// selects and updates a pathfinding type (eg: waypoint+obstacle avoid, A* optimal path, wall follow, Area awareness, raw compass?, etc)
//***************
void eAI::Think() {
	eInput * input;
	bool wasStopped;

// BEGIN FREEHILL DEBUG AI/player control
	input = &game.GetInput();
	if (input->MousePressed(SDL_BUTTON_LEFT)) {
		// TODO(?1/2?): funtionalize these two lines of getting mouse and camera, then converting to isometric
		// YES: make it part of the PLAYER class' input handling, and get rid of this block of code here
		eVec2 point = eVec2((float)input->GetMouseX(), (float)input->GetMouseY());
		point += game.GetCamera().CollisionModel().AbsBounds()[0];
		eMath::IsometricToCartesian(point.x, point.y);
		AddUserWaypoint(point);
	}

	if (input->KeyPressed(SDL_SCANCODE_C)) {
		pathingState = PATHTYPE_COMPASS;
		moveState = MOVETYPE_GOAL;
	} else if (input->KeyPressed(SDL_SCANCODE_W)) {
		pathingState = PATHTYPE_WALL;				
		moveState = MOVETYPE_GOAL;
	}
// END FREEHILL DEBUG AI/player control

	// only move with a waypoint
	if (currentWaypoint != nullptr) {

		wasStopped = !moving; 
		Move();
		UpdateKnownMap();		// FIXME: there is a user-input controlled function in here, it will not be called if UpdateWaypoint returns false

		// drop a trail waypoint (but never in a deadend that stopped the entity last frame)
		if (moving && !wasStopped && moveState != MOVETYPE_TRAIL && lastTrailTile != currentTile) {
			trail.PushFront(collisionModel.Origin());
			lastTrailTile = currentTile;
		}

		// check if goal is close enough to stop
		if (collisionModel.Origin().Compare(*currentWaypoint, goalRange)) {
			StopMoving();
			UpdateWaypoint(true);
		}

		// finalize the move
		if (collisionModel.Velocity() != vec2_zero) {
			moving = true;
			collisionModel.UpdateOrigin();
		}
	}
}

//***************
// eAI::Move
//***************
void eAI::Move() {

	// set the velocity
	if (pathingState == PATHTYPE_COMPASS)
		CompassFollow();
	else if (pathingState == PATHTYPE_WALL)
		WallFollow();
}

//******************
// eAI::WallFollow
// Determines the optimal movement vector to continue following a wall (as if a hand were placed on it)
// TODO: incorperate knownMap/stepRatio usage, directional bias usage?, more stopping conditions, goal waypoint short-circuit (like CompassFollow)
// DEBUG: default movement towards waypoint; default search for walls right-to-left using the first-found
//******************
void eAI::WallFollow() {
	decision_t	test;					// vector tested for optimal travel decision
	float		rotationAngle;			// cumulative amount the testVector has rotated in its search
	eQuat *		rotationDirection;		// wallSide affects the sweep direction that constitutes around-front of the entity
	bool		wallFollowing;
	bool		nearWall;

	if (!moving) { 
		forward.vector = *currentWaypoint - collisionModel.Origin();
		forward.vector.Normalize();
		moving = true;
	}

	CheckWalls(nullptr);
	if (wallSide != nullptr) {
		test = *wallSide;
		// initially sweep behind the entity to confirm wall location
		rotationDirection = wallSide == &left ? &rotateCounterClockwiseZ : &rotateClockwiseZ;
		wallFollowing = true;
	} else {
		test = forward;
		// initially sweep in front of entity if no wall is being followed
		rotationDirection = &rotateCounterClockwiseZ;
		wallFollowing = false;
	}

	nearWall = false;
	rotationAngle = 0.0f;
	while (rotationAngle < 360.0f) {
		CheckVectorPath(collisionModel.Origin() + (test.vector * speedHack), test);
		if (wallFollowing && nearWall && test.validSteps > 0) { 
			forward = test;
			CheckWalls(nullptr);
			break;					
		} else if (wallFollowing && !nearWall && test.validSteps == 0) {
			nearWall = true;
			// change to sweeping in front of entity for a path along the wall
			rotationDirection = wallSide == &left ? &rotateClockwiseZ : &rotateCounterClockwiseZ;
			rotationAngle = 0.0f;
		} else if (!wallFollowing && test.validSteps == 0) {
			wallSide = right.vector * test.vector >= left.vector * test.vector ? &right : &left;
			// continue sweeping in front of entity for a path along the wall
			rotationDirection = wallSide == &left ? &rotateClockwiseZ : &rotateCounterClockwiseZ;
			test = *wallSide;
			rotationAngle = 0.0f;
			wallFollowing = true;
			continue;
		}
		test.vector = *rotationDirection * test.vector;
		rotationAngle += ROTATION_INCREMENT;
	}

	if (wallSide != nullptr && (!nearWall || rotationAngle >= 360.0f))
		StopMoving();
	else 
		collisionModel.Velocity() = forward.vector * speedHack;
	// moveState may have changed, track the correct waypoint
//	UpdateWaypoint();
}

//******************
// eAI::CompassFollow
// Determines the optimal movement vector to reach the current waypoint
//******************
void eAI::CompassFollow() {
	decision_t	waypoint;				// from the sprite to the next waypoint
	decision_t	test;					// vector tested for optimal travel decision
	decision_t	best;					// optimal movement
	float		rotationAngle;			// cumulative amount the testVector has rotated in its search
	float		maxRotation;			// disallow vectors that backtrack if already moving
	float		weight;					// net bias for a decision about a test
	float		bestWeight;				// highest net result of all modifications to validSteps
	float		bias[4] = { 2.0f, 1.0f, 1.05f, 1.1f };	// { waypoint, left, right, forward }

	if (!moving) {
		test.vector = vec2_oneZero;
		maxRotation = 360.0f;
	} else {
		CheckWalls(bias);
		test = right;		// counter-clockwise sweep of 180 degree arc from right to left in the forward direction

//		if (moveState == MOVETYPE_GOAL)
		maxRotation = 180.0f;
//		else // moveState == MOVETYPE_TRAIL
//			maxRotation = 360.0f;				// FIXME/BUG: occasionally causes semi-permanent stuck-in-place jitter

		// stuck in a corner (look for the quickest and most waypoint-oriented way out)
		if ((forward.stepRatio == 0 && right.stepRatio == 0) || (forward.stepRatio == 0 && left.stepRatio == 0))
			*currentTile = VISITED_TILE;
	}

	waypoint.vector = *currentWaypoint - collisionModel.Origin();
	waypoint.vector.Normalize();

	bestWeight = 0.0f;
	rotationAngle = 0.0f;
	while (rotationAngle < maxRotation) {

		// check how clear the path is starting one step along it
		// and head straight for the waypoint if the test.vector path crosses extremely near it
		if (CheckVectorPath(collisionModel.Origin() + (test.vector * speedHack), test)) {
			if (CheckVectorPath(collisionModel.Origin() + (waypoint.vector * speedHack), waypoint))
				forward = waypoint;
			else
				forward = test;

			// initilize the new left and right, and their validSteps that'll be used next frame
			CheckWalls(nullptr);
			collisionModel.Velocity() = forward.vector * speedHack;
			return;
		}

		// FIXME/BUG: trail waypoint orbits or cannot attain sometimes (bad corner, whatever)
		// SOMEWHAT fixed by putting a trail waypoint on each new tile (its never too far to navigate straight back to)
		// SOMEWHAT fixed by modulating speed based on waypoint proximity, except backtracking looked weird so that was removed

		// give the path a bias to help set priority
		weight = test.validSteps;
		weight += (test.vector * waypoint.vector) * bias[0];
		if (moving) {
			weight += (test.vector * left.vector) * bias[1];
			weight += (test.vector * right.vector) * bias[2];
			weight += (test.vector * forward.vector) * bias[3];
		}

		// more new tiles always beats better overall weight
		if (test.stepRatio > best.stepRatio) {
			bestWeight = weight;
			best = test;
		} else if (test.stepRatio == best.stepRatio  && weight > bestWeight) {
			bestWeight = weight;
			best = test;
		}
		test.vector = rotateCounterClockwiseZ * test.vector;
		rotationAngle += ROTATION_INCREMENT;
	}

	if (moveState == MOVETYPE_GOAL && best.stepRatio == 0) {	// deadlocked, begin deadend protocols (ie follow the trail now)
		StopMoving();
		moveState = MOVETYPE_TRAIL;
	} else if (moveState == MOVETYPE_TRAIL && best.stepRatio > 0) {
		StopMoving();
		moveState = MOVETYPE_GOAL;
	} else if (moveState == MOVETYPE_TRAIL && best.validSteps == 0) {
		StopMoving();
	} else {
		forward = best;
		// initilize the new left and right, and their validSteps that'll be used next frame
		CheckWalls(nullptr);
		collisionModel.Velocity() = forward.vector * speedHack;
	}
	// moveState may have changed, track the correct waypoint
	UpdateWaypoint();
}

//******************
// eAI::CheckVectorPath
// determines the state of the entity's position for the next few frames
// return true if a future position using along is near the waypoint
// TODO: make this a proper area/volume trace
//******************
bool eAI::CheckVectorPath(eVec2 from, decision_t & along) {
	eVec2 testPoint;
	float newSteps;

	along.validSteps = 0.0f;
	along.stepRatio = 0.0f;
	newSteps = 0.0f;
	while (along.validSteps < maxSteps) {

		// forward test point (starts on circle circumscribing the sprite bounding box)
		testPoint.x = from.x + (collisionRadius * along.vector.x);
		testPoint.y = from.y + (collisionRadius * along.vector.y);

		// check for collision
		if (!(game.GetMap().IsValid(testPoint)))
			break;

		// forward test point rotated clockwise 90 degrees
		testPoint.x = from.x + (collisionRadius * along.vector.y);
		testPoint.y = from.y - (collisionRadius * along.vector.x);

		// check for collision
		if (!(game.GetMap().IsValid(testPoint)))
			break;

		// forward test point rotated counter-clockwise 90 degrees
		testPoint.x = from.x - (collisionRadius * along.vector.y);
		testPoint.y = from.y + (collisionRadius * along.vector.x);

		// check for collision
		if (!(game.GetMap().IsValid(testPoint)))
			break;

		// all test points validated
		along.validSteps++;

		// check if the step falls on an unexplored tile
		if (knownMap.Index(from) == UNKNOWN_TILE)
			newSteps++;

		// check if the goal waypoint is near the center of the validated test position
		if (moveState == MOVETYPE_GOAL && from.Compare(*currentWaypoint, goalRange))
			return true;

		// move to check validity of next position
		from += along.vector * speedHack;
	}

	if (along.validSteps == 0.0f)
		along.stepRatio = 0.0f;
	else
		along.stepRatio = newSteps / along.validSteps;

	return false;
}

//**************
// eAI::CheckWalls
// assigns the vectors perpendicular to the forward vector
// and checks if the range along them has significantly changed
//**************
void eAI::CheckWalls(float * bias) {
	float oldLeftSteps = left.validSteps;
	float oldRightSteps = right.validSteps;

	static const float stepIncreaseThreshold = 2.0f;

	left.vector.Set(-forward.vector.y, forward.vector.x);	// forward rotated 90 degrees counter-clockwise
	right.vector.Set(forward.vector.y, -forward.vector.x);	// forward rotated 90 degrees clockwise

	CheckVectorPath(collisionModel.Origin() + (forward.vector * speedHack), forward);
	CheckVectorPath(collisionModel.Origin() + (left.vector * speedHack), left);
	CheckVectorPath(collisionModel.Origin() + (right.vector * speedHack), right);

	if (bias == nullptr)
		return;

	// bias[0] == waypoint bias remains unchanged because if * test.stepRatio; or * waypoint.stepRatio; then trail-less backtracking occurs (bad)
	bias[1] *= (left.validSteps >= oldLeftSteps + stepIncreaseThreshold) * left.stepRatio;		// left path opened
	bias[2] *= (right.validSteps >= oldRightSteps + stepIncreaseThreshold) * right.stepRatio;	// right path opened
	bias[3] *= !(forward.validSteps == 0.0f) * forward.stepRatio;								// forward path not closed
}


//******************
// eAI::CheckFogOfWar
// TODO: have the map object check if ANY of the team entities have visited a tile about to be drawn, if not draw black,
// if so, then the map goes through all entities calling CheckFogOfWar, if ONE returns true then it'll stop sweep and draw bright,
// if none return true by the end of the sweep, then draw dim ( reduce sweep time by using a locational Potential_Visible_Set )
//******************
bool eAI::CheckFogOfWar(const eVec2 & point) const {
	eVec2 lineOfSight;

	lineOfSight = point - collisionModel.Origin();
	return lineOfSight.LengthSquared() <= sightRange;
}

//******************
// eAI::AddUserWaypoint
// TODO: use SpatialIndexGrid::Validate(point) to snap a waypoint onto the nearest
// in-bounds map point (like Age of Empires flags)
//******************
void eAI::AddUserWaypoint(const eVec2 & waypoint) {

	// TODO: disallow waypoints on collision tiles and off-map
	// allow other entities to become waypoints (that move)
	// moving waypoints would need to have their info updated in the deque (hence pointers instead of copies)
	// of course that could be a separate category altogether

	if (game.GetMap().IsValid(waypoint)) {
		goals.PushFront(waypoint);
		UpdateWaypoint();
	}
}

//******************
// eAI::UpdateWaypoint
//******************
void eAI::UpdateWaypoint(bool getNext) {
	switch (moveState) {
		case MOVETYPE_GOAL: {
			if (getNext && !goals.IsEmpty()) {
				goals.PopBack();
				trail.Clear();
			}
			CheckTrail();
			if (!goals.IsEmpty()) {
				currentWaypoint = &goals.Back()->Data();
				return;
			}
			currentWaypoint = nullptr;
			return;
		}
		case MOVETYPE_TRAIL: {
			if (getNext && !trail.IsEmpty())
				trail.PopFront();
			if (!CheckTrail()) {
				currentWaypoint = &trail.Front()->Data();
				return;
			} else if (!goals.IsEmpty()) {
				moveState = MOVETYPE_GOAL;
				currentWaypoint = &goals.Back()->Data();
				return;
			}
			currentWaypoint = nullptr;
			return;
		}
		default: {		// DEBUG: currently for PATHTYPE_WALL
						// FIXME/TODO: have PATHTYPE_WALL pay attention to knownMap and trail waypoints too
			if (getNext && !goals.IsEmpty()) {
				goals.PopBack();
				trail.Clear();
			}
			CheckTrail();
			if (!goals.IsEmpty()) {
				currentWaypoint = &goals.Back()->Data();
				return;
			}
			currentWaypoint = nullptr;
			return;
		}
	}
}

//******************
// eAI::CheckTrail
// returns false if the entity should fresh-start goal pathfinding
//******************
bool eAI::CheckTrail() {
	eInput * input;

	input = &game.GetInput();
	if (trail.IsEmpty() || game.debugFlags.KNOWN_MAP_CLEAR && input->KeyPressed(SDL_SCANCODE_R)) {
		knownMap.ClearAllCells();
		lastTrailTile = nullptr;
		return true;
	}
	return false;
}

//******************
// eAI::UpdateKnownMap
// marks the currentTile as VISITED_TILE, clears out un-needed trail waypoints,
// and resets tiles around the current goal waypoint to UNKNOWN_TILE
//******************
void eAI::UpdateKnownMap() {
	byte_t * checkTile;
	int row, column;
	int startRow, startCol;
	int endRow, endCol;
	int tileResetRange;		// size of the box around the goal to set tiles to UNKNOWN_TILE

	// mark the tile to help future movement decisions
	// DEBUG: only needs to be more then **half-way** onto a new tile
	// to set the currentTile as previousTile and VISITED_TILE,
	// instead of completely off the tile (via a full absBounds check agains the eTile bounds)
	checkTile = &knownMap.Index(collisionModel.Origin());
	if (checkTile != currentTile) {
		previousTile = currentTile;
		*previousTile = VISITED_TILE;
		currentTile = checkTile;
	}

	tileResetRange = 0;
	if (!CheckTrail()) {
		
		// solid-box of tiles at the tileResetRange centered on **the current goal waypoint** to to reset the knownMap
		// FIXME: this Length() call is very costly and shouldn't run each frame
		// SOLUTION: find a workaround for using the knownMap at all, and/or prevent the current "orbit" behavior
		// that the AI displays when attempting to attain a goal (ie sometimes there's an opening that's missed in favor
		// of maintaining the directional weight)
		if (!goals.IsEmpty()) {
			tileResetRange = (int)((goals.Back()->Data() - collisionModel.Origin()).Length() / (knownMap.CellWidth() * 2));

			// find the knownMap row and column of the current goal waypoint
			knownMap.Index(goals.Back()->Data(), row, column);

			// set initial bounding box top-left and bottom-right indexes within knownMap
			startRow = row - (tileResetRange / 2);
			startCol = column - (tileResetRange / 2);
			endRow = row + (tileResetRange / 2);
			endCol = column + (tileResetRange / 2);

			// snap bounding box rows & columns within range of the tileMap area
			if (startRow < 0)
				startRow = 0;
			if (startCol < 0)
				startCol = 0;

			if (endRow >= knownMap.Rows())
				endRow = knownMap.Rows() - 1;
			if (endCol >= knownMap.Columns())
				endCol = knownMap.Columns() - 1;

			// reset tiles within the bounding box
			row = startRow;
			column = startCol;
			while (row <= endRow) {
				knownMap.Index(row, column) = UNKNOWN_TILE;

				column++;
				if (column > endCol) {
					column = startCol;
					row++;
				}
			}
		}
	}

	// pop all trail waypoints that no longer fall on VISITED_TILEs
	while (!trail.IsEmpty()) {
		if (knownMap.Index(trail.Back()->Data()) == UNKNOWN_TILE)
			trail.PopBack();
		else
			break;
	}
}

//***************
// eAI::Draw
// TODO: make this draw independent of eEntity::Draw()
//***************
void eAI::Draw() {
	eEntity::Draw();
}

//******************
// eAI::DebugDraw
//******************
void eAI::DebugDraw() {
	DrawGoalWaypoints();
	DrawKnownMap();
	DrawCollisionCircle();
	DrawTrailWaypoints();
}

//******************
// eAI::DrawGoalWaypoints
//******************
void eAI::DrawGoalWaypoints() {
	eNode<eVec2> * iterator;
	eVec2 goalPoint;

	if (!game.debugFlags.GOAL_WAYPOINTS)
		return;

	auto & cameraBounds = game.GetCamera().CollisionModel().AbsBounds();
	for (iterator = goals.Back(); iterator != nullptr; iterator = iterator->Next()) {
		goalPoint = iterator->Data();
		goalPoint.SnapInt();
		eBounds goalBounds = eBounds(goalPoint).ExpandSelf(8);
		game.GetRenderer().DrawIsometricRect(redColor, goalBounds, true);
	}
}

//******************
// eAI::DrawTrailWaypoints
//******************
void eAI::DrawTrailWaypoints() {
	eNode<eVec2> * iterator;
	eVec2 trailPoint;

	if (!game.debugFlags.TRAIL_WAYPOINTS)
		return;

	for(iterator = trail.Front(); iterator != nullptr; iterator = iterator->Prev()) {
		trailPoint = iterator->Data();
		trailPoint.SnapInt();
		eBounds trailBounds = eBounds(trailPoint).ExpandSelf(4);
		game.GetRenderer().DrawIsometricRect(greenColor, trailBounds, RENDERTYPE_DYNAMIC);
	}
}

//******************
// eAI::DrawCollisionCircle
// draws the forward sweep angle checked for collision prediction in any-angle pathfinding
// FIXME: draw a partial circle in eRenderer instead
//******************
void eAI::DrawCollisionCircle() {
/*
	eVec2 debugVector;
	eVec2 debugPoint;
	float rotationAngle;
	int pink[3] = { 255, 0, 255 };
	int blue[3] = { 0,0,255 };
	int * color;
*/
	if (!game.debugFlags.COLLISION)
		return;
/*
	// draws one pixel for each point on the current collision circle 
	if (moveState == MOVETYPE_GOAL)
		color = pink;
	else
		color = blue;

	debugVector = vec2_oneZero;
	rotationAngle = 0.0f;
	while (rotationAngle < 360.0f) {
		if (collisionModel.Velocity() * debugVector >= 0) {
			debugPoint = collisionModel.Origin() + (debugVector * collisionRadius) - game.GetCamera().CollisionModel().AbsBounds()[0];
			debugPoint.SnapInt();
*/
			// DEBUG: just test the iso-collision bounds for now
			game.GetRenderer().DrawIsometricRect(yellowColor, collisionModel.AbsBounds(), RENDERTYPE_DYNAMIC);
//			game.GetRenderer().DrawPixel(debugPoint, color[0], color[1], color[2]);
/*
		}
		debugVector = rotateCounterClockwiseZ * debugVector;
		rotationAngle += ROTATION_INCREMENT;
	}
*/
}

//******************
// eAI::DrawKnownMap
// TODO: check collision/draw layers, and draw debug rects over visited (and visible) tiles instead of entire cells.
//******************
void eAI::DrawKnownMap() const {
	auto & tileMap = game.GetMap().TileMap();
	auto & visibleCells = game.GetMap().VisibleCells();
	for (auto & visibleCell : visibleCells) {
		if (knownMap.Index(visibleCell.first, visibleCell.second) == VISITED_TILE) {
				auto cellBounds = tileMap.Index(visibleCell.first, visibleCell.second).AbsBounds();
				game.GetRenderer().DrawIsometricRect(pinkColor, cellBounds, RENDERTYPE_DYNAMIC);
		}
	}

}