#ifndef EVIL_BOUNDS_H
#define EVIL_BOUNDS_H

#include "Math.h"

// TODO: traslating the model-bounds yields the world-relative bounds
// Doom declares a bounds object AND A absBounds object for each clipmodel (which has a pointer to an entity)
// AS WELL AS an idVec3 origin that is moving around
// this essentially gives each clipmodel/entity its own coordinate system
/*
FROM Clip.cpp (idClipModel::Link)
absBounds[0] = bounds[0] + origin;
absBounds[1] = bounds[1] + origin;
// every single time the origin is moved the entity has to be relinked (otherwise the entity isn't updated due to origin mismatch)
*/

// Develop this class to account for bounds rotated about an origin
// to allow for better shape-fitting of rotated sprite animations
// never translate the model-bounds (only the absBounds); allow rotation of the model-bounds
// only ever get the model-bounds radius/dimensions
// "size" could be a vector from mins to maxs (the diagonal of the box == the diameter of the circumscribed circle)

//**********************************
//			eBounds
// 2D Axis-Aligned bounding box
//**********************************
class eBounds {
public:
					eBounds();
	explicit		eBounds(const eVec2 & mins, const eVec2 & maxs);
	explicit		eBounds(const eVec2 & point);

	const eVec2 &	operator[](const int index) const;
	eVec2 &			operator[](const int index);
	eBounds			operator+(const eVec2 & translation) const;				
	eBounds &		operator+=(const eVec2 & translation);						

	eBounds			Translate(const eVec2 & translation) const;		
	eBounds &		TranslateSelf(const eVec2 & translation);		
	eBounds			Expand(const float range) const;				
	eBounds &		ExpandSelf(const float range);					

	void			Zero();											
	eVec2			Center() const;									
	float			Radius() const;									

	float			Width() const;
	float			Height() const;
/*
	bool			ContainsPoint(const eVec2 & point) const;			// includes touching
	bool			IntersectsBounds(const eBounds & a) const;			// includes touching
	bool			LineIntersection(const eVec2 & start, const eVec2 & end) const;
	// intersection point is start + dir * scale
	bool			RayIntersection(const eVec2 & start, const eVec2 & dir, float & scale) const;
*/

private:

	eVec2			bounds[2];			// mins == bounds[0] and maxs == bounds[1]
};

//*************
// eBounds::eBounds
// single point on origin (0, 0)
//*************
inline eBounds::eBounds() {
	bounds[0] = ZERO_VEC2;
	bounds[1] = ZERO_VEC2;
}

//*************
// eBounds::eBounds
//*************
inline eBounds::eBounds(const eVec2 & mins, const eVec2 & maxs) {
	bounds[0] = mins;
	bounds[1] = maxs;
}

//*************
// eBounds::eBounds
//*************
inline eBounds::eBounds(const eVec2 & point) {
	bounds[0] = point;
	bounds[1] = point;
}

//*************
// eBounds::operator[]
// undefined behavior for index out of bounds
//*************
inline const eVec2 & eBounds::operator[](const int index) const {
	return bounds[index];
}

//*************
// eBounds::operator[]
// undefined behavior for index out of bounds
//*************
inline eVec2 & eBounds::operator[](const int index) {
	return bounds[index];
}

//*************
// eBounds::operator+
// returns translated bounds
//*************
inline eBounds eBounds::operator+(const eVec2 & translation) const {
	return eBounds(bounds[0] + translation, bounds[1] + translation);
}

//*************
// eBounds::operator+=
// translate the bounds
//*************
inline eBounds & eBounds::operator+=(const eVec2 & translation) {
	bounds[0] += translation;
	bounds[1] += translation;
	return *this;
}

//*************
// eBounds::Zero
// single point at origin
//*************
inline void eBounds::Zero() {
	bounds[0].x = bounds[0].y = bounds[1].x = bounds[1].y = 0.0f;
}

//*************
// eBounds::Center
// returns center of bounds
//*************
inline eVec2 eBounds::Center() const {
	return eVec2((bounds[1].x + bounds[0].x) * 0.5f, (bounds[1].y + bounds[0].y) * 0.5f);
}

//*************
// eBounds::Radius
// returns the radius relative to the bounds origin (0,0)
//*************
inline float eBounds::Radius() const {
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for (i = 0; i < 2; i++) {
		b0 = (float)SDL_fabs(bounds[0][i]);
		b1 = (float)SDL_fabs(bounds[1][i]);

		if (b0 > b1)
			total += b0 * b0;
		else 
			total += b1 * b1;
	}
	return SDL_sqrtf(total);
}

//*************
// eBounds::Translate
// return translated bounds
//*************
inline eBounds eBounds::Translate(const eVec2 & translation) const {
	return eBounds(bounds[0] + translation, bounds[1] + translation);
}

//*************
// eBounds::TranslateSelf
// translate this bounds
//*************
inline eBounds & eBounds::TranslateSelf(const eVec2 & translation) {
	bounds[0] += translation;
	bounds[1] += translation;
	return *this;
}

//*************
// eBounds::Expand
// return bounds expanded in all directions with the given value
//*************
inline eBounds eBounds::Expand(const float range) const {
	return eBounds(eVec2(bounds[0].x - range, bounds[0].y - range),
		eVec2(bounds[1].x + range, bounds[1].y + range));
}

//*************
// eBounds::ExpandSelf
// expand bounds in all directions with the given value
//*************
inline eBounds & eBounds::ExpandSelf(const float range) {
	bounds[0].x -= range;
	bounds[0].y -= range;
	bounds[1].x += range;
	bounds[1].y += range;
	return *this;
}

//*************
// eBounds::Width
// FIXME: only works for non-rotated bounds
//*************
inline float eBounds::Width() const {
	return bounds[1].x - bounds[0].x;
}

//*************
// eBounds::Height
// FIXME: only works for non-rotated bounds
//*************
inline float eBounds::Height() const {
	return bounds[1].y - bounds[0].y;
}

#endif  /* EVIL_BOUNDS_H */


