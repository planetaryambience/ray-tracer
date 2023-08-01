/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The cylinder class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Cylinder.h"
#include <math.h>

/**
* Cylinder's intersection method.  The input is a ray.
*/
float Cylinder::intersect(glm::vec3 p0, glm::vec3 dir)
{
    // Lecture 8, page 40
    float a = (dir.x * dir.x) + (dir.z * dir.z);
    float b = 2 * ((dir.x * (p0.x - center.x)) + (dir.z * (p0.z - center.z)));
    float c = ((p0.x - center.x) * (p0.x - center.x))
              + ((p0.z - center.z) * (p0.z - center.z))
              - (radius * radius);

    float discriminant = (b * b) - (4 * a * c);
    if (discriminant < 0.0001) return -1.0;

    float t1 = (-b + sqrt(discriminant)) / (2 * a);     // positive quadratic formula
    float t2 = (-b - sqrt(discriminant)) / (2 * a);     // negative quadratic formula

    // check which point is closest point of intersection
    float t = t1;
    if (t1 >= t2) {
        t1 = t2;
        t2 = t;
    }

    float y1 = p0.y + (t1 * dir.y);
    float y2 = p0.y + (t2 * dir.y);

    if ((y1 > center.y) && (y1 < (center.y + height))) {
        return (t1 < t2) ? t1 : t2;
    } else if ((y1 > (center.y + height)) && (y2 < (center.y + height))) {
        return (center.y + height - p0.y) / dir.y;
    } else if (y2 > center.y && y2 < (center.y + height)) {
        return t2;
    } else {
        return -1.0;
    }
}


/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the cylinder.
*/
glm::vec3 Cylinder::normal(glm::vec3 p)
{
    // top cap
    if (p.y >= (center.y + height) - 0.1)
        return glm::vec3(0, 1, 0);

    // normalized vector
    glm::vec3 n((p.x - center.x) / radius, 0, (p.z - center.z) / radius);
    return n;
}
