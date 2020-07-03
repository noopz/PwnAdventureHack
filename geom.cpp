#include <Windows.h>
#include "geom.h"

// PA3 is a D3D game. So this is unused but will slowly add to the collection of useful functions for
// doing geometry operations
bool GLWorldToScreen(Vector3 pos, Vector3& screen, float matrix[16], int windowWidth, int windowHeight)
{
	//Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
	Vector4 clipCoords;
	clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
	clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
	clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
	clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

	// If the W is small enough then the position is either behind too close to the near plane
	if (clipCoords.w < 0.1f)
		return false;

	//perspective division, dividing by clip.W = Normalized Device Coordinates
	Vector3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
	return true;
}

// For details about quaternions and converting to Euler angles:
// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
// Typically in 3D games, a +/- Z value moves you forward in space. In the case of PwnAdventure3
// that value is +Z. This means that the bottom row of the orthogonal matrix is used, or put in another way.
// Multiplying the orthogonl matrix by the following:
// Forward (z): (0,0,1)
// Up (y): (0,1,0)
// Left (x): (1,0,0)
// In this case we're only implementing the result for the forward operation
Vector3 ForwardVec3FromQuat(Vector4* quat)
{
	Vector3 directionFromQuat;
	directionFromQuat.x = (1 - 2 * (quat->y * quat->y + quat->z * quat->z));
	directionFromQuat.y = (2 * (quat->x * quat->y + quat->w * quat->z));
	directionFromQuat.z = (2 * (quat->x * quat->z - quat->w * quat->y));

	return directionFromQuat;
}