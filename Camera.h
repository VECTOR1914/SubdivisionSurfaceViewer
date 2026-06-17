/* Class Camera */
 #ifndef Camera_H
#define Camera_H

#include <vector>
#include <string>
#include <fstream>

/* Class Camera */
 
class Camera
{
public: 	

	void resetVars();
		
	float camera_x;       // pan offset X (right-drag)
	float camera_y;       // pan offset Y (right-drag)

	float camera_rotx;    // orbit angle around X-axis
	float camera_roty;    // orbit angle around Y-axis
	float camera_rotz;    // (unused, kept for compat)

	float camera_distance; // distance from look-at target
};

#endif
