#ifndef ENTITY_H
#define ENTITY_H

#include "Game.h"

class Game;
class Map;

// movement decision values
#define MAX_STEPS 5
#define STEP_INCRESE_THRESHOLD 2
#define ROTATION_INCREMENT 1.0f
#define RIGHT_WALL_OPENED BIT(1)
#define LEFT_WALL_OPENED BIT(2)
#define FORWARD_WALL_HIT BIT(3)
#define RIGHT_BIAS 1.05f
#define LEFT_BIAS 1.0f
#define FORWARD_BIAS 1.1f
#define WAYPOINT_BIAS 2.0f

class Entity {

private:

	typedef struct decision_s {
		eVec2 vector = ZERO_VEC2;
		int validSteps = 0;			// collision-free steps that could be taken along the vector
		int newSteps = 0;			// valid steps that land on previously unvisited tiles
	} decision_t;
	
	void			CollisionCheck(bool horizontal, bool vertical);	// FIXME: should this belong to the Map class?
	void			CheckFogOfWar();
	void			UpdateMovement();
	bool			CheckMovement(eVec2 from, decision_t & along);
	void			CheckWalls(size_t & walls);
	bool			CheckFloor();
	void			SetNextWaypoint();
	void			StopMoving();
	void			RemoveWaypoint();
	void			SetKnownMapValue(const eVec2 & point, int value);

	void			CheckTouch(bool self, bool horizontal, bool vertical);
	void			PrintSensors();
	void			UpdatePosition();
	void			UpdateCenter();

	void			DrawPixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b); // DEBUG: collision circle testing

	Game*			game;
	SDL_Surface*	sprite;
	SDL_Rect		sight;				// TODO: make sight and touch part of organized structs?
	int				knownMap[MAX_MAP_SIZE][MAX_MAP_SIZE];	// matches 1:1 the size of the tileMap in Map.h and the 
	int				knownMapRows;							// matches 1:1 the mapRows in Map.h
	int				knownMapCols;							// matches 1:1 the mapCols in Map.h
	eVec2			spritePos;
	eVec2			spriteCenter;
	unsigned int	moveState;
	unsigned int	oldMoveState;
	int				speed;
	int				size;
	float			collisionRadius;		// circular collision radius for prediction when using line of sight
	float			waypointRange;			// acceptable range to snap sprite position to user-defined waypoint
	int				sightRange;				// range of drawable visibility (fog of war)
	int				touchRange;				// range beyond body to trigger touch sensors

	// TODO: incorperate animation handling instead of static images
	int				firstFrame;
	int				lastFrame;
	int				currentFrame;
	int				frameDelay;
	int				frameDelayCount;

	unsigned int	localTouch;		// on-sprite sensors
	unsigned int	oldTouch;		// off-sprite sensors
	unsigned int	touch;			// off-sprite sensors
	unsigned int	watch_touch;	// marks forward-sensors to watch given the moveState

	// these nodes function as the top of their stack for this entity
	EvilDeque<eVec2> trail;		// AI-defined AI movement tracking ( points into Game.h's waypointNodes array )
	EvilDeque<eVec2> goals;		// User-defined AI movement goals ( points into Game.h's waypointNodes array )
	eVec2 * currentWaypoint;		// simplifies switching between the deque being tracked

	decision_t		forward;				// currently used movement vector
	decision_t		left;					// perpendicular to forward_v counter-clockwise
	decision_t		right;					// perpendicular to forward_v clockwise
	eQuat			rotationQuat_Z;			// to rotate any vector about z-axis

	bool			atWaypoint;
	bool			moving;

	enum sensors {

		TOP_LEFT		= 1,
		TOP_RIGHT		= 2,
		RIGHT_TOP		= 4,
		RIGHT_BOTTOM	= 8,
		BOTTOM_RIGHT	= 16,
		BOTTOM_LEFT		= 32,
		LEFT_BOTTOM		= 64,
		LEFT_TOP		= 128

	};

	// move_state bits for direction and collision decisions
	enum movement {

		MOVE_LEFT		= 1,			// wall-follow
		MOVE_RIGHT		= 2,			// wall-follow
		MOVE_UP			= 4,			// wall-follow
		MOVE_DOWN		= 8,			// wall-follow
		MOVE_TO_GOAL	= 16,			// waypoint tracking
		MOVE_TO_TRAIL	= 32			// waypoint tracking

	};

public:

					Entity();

	bool			Init(char fileName[], bool key, Game * const game);
	void			Free();
	void			Spawn();
	void			Update();
	void			Move();		// give this a broader functionality beyond pure AI movement (ie player input)
	int				GetKnownMapValue(int row, int column) const;
	int				GetKnownMapValue(const eVec2 & point) const;
	void			AddUserWaypoint(const eVec2 & waypoint);
	// TODO: add GetTrailTop() function for other entities to track this entity's path 
	// or something similar (ie the enemy doesn't know where the entity is going, only where its been
	// and picks up the trail where it last saw the entity
	void			SetPosition(const eVec2 & point);
	const eVec2 &	GetCenter() const;
};

#endif /* ENTITY_H */