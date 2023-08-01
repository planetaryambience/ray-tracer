//  ======================================
//
//  COSC363: assignment 2
//  02/06/2023
//
//  ======================================

#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Plane.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "SceneObject.h"
#include "Ray.h"
#include "TextureBMP.h"
#include <GL/freeglut.h>
#include <glm/gtc/random.hpp>

using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;
float PI = 3.141592653;

vector<SceneObject*> sceneObjects;
TextureBMP texture;

glm::vec3 sphereCenter = glm::vec3(6.5, -3.4, -73.0);

// compute colour value by tracing ray and finding closest point of intersection (Lab 6)
glm::vec3 trace(Ray ray, int step)
{
    glm::vec3 backgroundCol(0);
    glm::vec3 lightPos(-15, 40, -1);

    glm::vec3 color(0);
    SceneObject* obj;

    ray.closestPt(sceneObjects);                        // compare the ray with all objects in the scene
    if (ray.index == -1) return backgroundCol;          // no intersection
    obj = sceneObjects[ray.index];                      // object on which the closest point of intersection is found

    if (ray.index == 0)
    {
        // checkered pattern for floor plane (based on code from Lab 7)
        int checkerSize = 3;
        int iz = (ray.hit.z) / checkerSize;
        int ix = (ray.hit.x) / checkerSize;
        int k = (iz + ix) % 2;                      // 2 colors

        if (ray.hit.x >= 0) {
            if (k == 0) k = 1;
            else k = 0;
        }

        if (k == 0) color = glm::vec3(0.5, 0.5, 0.5);
        else color = glm::vec3(1, 1, 1);
        obj->setColor(color);

    }

    color = obj->lighting(lightPos, -ray.dir, ray.hit);

    // texture mapping
    if (ray.index == 10)
    {
        glm::vec3 d = glm::normalize(ray.hit - sphereCenter);
        float u = 0.5 + (atan2(d.z, d.x) / (2 * PI));
        float v = 0.5 + (asin(d.y) / PI);
        color = texture.getColorAt(u, v);
    }

    // shadows
    glm::vec3 lightVec = lightPos - ray.hit;
    float lightDist = glm::length(lightVec);
    Ray shadowRay(ray.hit, lightVec);
    shadowRay.closestPt(sceneObjects);

    if (shadowRay.index > -1 && shadowRay.dist < lightDist) {
        float ambient = 0.2f;

        // object hit by shadow ray
        SceneObject* shadowObj;
        shadowObj = sceneObjects[shadowRay.index];

        if (shadowObj->isTransparent() || shadowObj->isRefractive()) {
            ambient = 0.5f;
        }

        if (obj->isTransparent()) color = ambient * color;
        else color = ambient * obj->getColor();
    }

    // spotlight
    glm::vec3 spotlightPos(10, 15, -70);
    float spotlightAngle = 0.3490659;

    glm::vec3 spotlightVec = spotlightPos - ray.hit;
    float spotlightDist = glm::length(spotlightVec);
    Ray shadowRay2(ray.hit, spotlightVec);
    shadowRay2.closestPt(sceneObjects);

    if (shadowRay2.index > -1 && shadowRay2.dist < spotlightDist && ray.index >= 10) {
            color = 0.2f * color;
    } else {
        // if the angle between the shadow ray d and the spot direction sd is greater than cutoff the object is in shadow.
        glm::vec3 spotlightDir(-0.1, -0.5, 0);
        float ab = glm::dot(-shadowRay2.dir, spotlightDir);
        float a = glm::length(-shadowRay2.dir);
        float b = glm::length(spotlightDir);
        float angle = acos(ab / (a * b));

        if (angle > spotlightAngle) {
            color = color * 0.7f;
        }
    }

    // reflective
    if (obj->isReflective() && step < MAX_STEPS)
    {
        float rho = obj->getReflectionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
        color = color + (rho * reflectedColor);
    }

    // refractive
    if (obj->isRefractive() && step < MAX_STEPS)
    {
        float eta = 1 / obj->getRefractiveIndex();

        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 refractedDir = glm::refract(ray.dir, normalVec, eta);
        Ray refractedRay(ray.hit, refractedDir);
        refractedRay.closestPt(sceneObjects);

        glm::vec3 normalRefrVec = obj->normal(refractedRay.hit);
        glm::vec3 refractedRayDir = glm::refract(refractedDir, -normalRefrVec, 1.0f / eta);
        Ray refrRay(refractedRay.hit, refractedRayDir);
        color = color + trace(refrRay, step + 1);
    }

    // transparent
    if (obj->isTransparent() && step < MAX_STEPS) {
        Ray passingRay(ray.hit, ray.dir);
        glm::vec3 transparentColor = trace(passingRay, step + 1);
        // color = (transparentColor * obj->getTransparencyCoeff()) + ((1-obj->getTransparencyCoeff()) * color);
        color += (transparentColor * obj->getTransparencyCoeff());
    }

    // fog
    float z1 = -70;
    float z2 = -250;
    glm::vec3 fogColor(0.5, 0.2, 0.8);
    float l = (ray.hit.z - z1) / (z2 - z1);
    color = (1 - l) * color + l * fogColor;

    return color;
}

// main display module (Lab 6)
void display()
{
    float xp, yp;                           // grid point
    float cellX = (XMAX - XMIN) / NUMDIV;   //cell width
    float cellY = (YMAX - YMIN) / NUMDIV;   //cell height
    glm::vec3 eye(0., 0., 0.);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);                      // each cell is a tiny quad.
    for (int i = 0; i < NUMDIV; i++)        // scan every cell of the image plane
    {
        xp = XMIN + i * cellX;
        for (int j = 0; j < NUMDIV; j++)
        {
            yp = YMIN + j * cellY;

            //glm::vec3 dir(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST);  // direction of the primary ray
            //Ray ray = Ray(eye, dir);
            //glm::vec3 col = trace(ray, 1);                  // trace the primary ray and get the colour value

            // anti aliasing
            glm::vec3 dir1((xp + 0.25 * cellX), (yp + 0.25 * cellY), -EDIST);     // split cell into four
            glm::vec3 dir2((xp + 0.25 * cellX), (yp - 0.25 * cellY), -EDIST);
            glm::vec3 dir3((xp - 0.25 * cellX), (yp + 0.25 * cellY), -EDIST);
            glm::vec3 dir4((xp - 0.25 * cellX), (yp - 0.25 * cellY), -EDIST);

            Ray ray1 = Ray(eye, dir1);
            Ray ray2 = Ray(eye, dir2);
            Ray ray3 = Ray(eye, dir3);
            Ray ray4 = Ray(eye, dir4);

            glm::vec3 col1 = trace(ray1, 1);
            glm::vec3 col2 = trace(ray2, 1);
            glm::vec3 col3 = trace(ray3, 1);
            glm::vec3 col4 = trace(ray4, 1);

            glm::vec3 col;
            // get average colours
            col.g = (col1.g + col2.g + col3.g + col4.g) / 4.0f;
            col.b = (col1.b + col2.b + col3.b + col4.b) / 4.0f;
            col.r = (col1.r + col2.r + col3.r + col4.r) / 4.0f;

            glColor3f(col.r, col.g, col.b);
            glVertex2f(xp, yp);                                         // draw each cell with its color value
            glVertex2f(xp + cellX, yp);
            glVertex2f(xp + cellX, yp + cellY);
            glVertex2f(xp, yp + cellY);
        }
    }
    glEnd();
    glFlush();
}

// initialize scene, create scene objects
void initialize()
{
    texture = TextureBMP("Earth.bmp");

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

    glClearColor(0, 0, 0, 1);

    // box planes
    Plane* floor = new Plane(
        glm::vec3(-30., -15, 0),        // point A
        glm::vec3(30., -15, 0),         // point B
        glm::vec3(30., -15, -250),      // point C
        glm::vec3(-30., -15, -250));    // point D
    floor->setSpecularity(false);
    sceneObjects.push_back(floor);

    Plane* wall1 = new Plane(
        glm::vec3(-30., -15, -250),
        glm::vec3(30., -15, -250),
        glm::vec3(30., 45, -250),
        glm::vec3(-30., 45, -250));
    wall1->setColor(glm::vec3(0.2, 0.2, 0.2));
    wall1->setSpecularity(false);
    wall1->setReflectivity(true);
    sceneObjects.push_back(wall1);

    Plane* wall2 = new Plane(
        glm::vec3(-30., -15, 0),
        glm::vec3(-30., -15, -250),
        glm::vec3(-30., 45, -250),
        glm::vec3(-30., 45, 0));
    wall2->setColor(glm::vec3(0.2, 0.5, 0.8));
    wall2->setSpecularity(false);
    sceneObjects.push_back(wall2);

    Plane* wall3 = new Plane(
        glm::vec3(30., -15, -250),
        glm::vec3(30., -15, 0),
        glm::vec3(30., 45, 0),
        glm::vec3(30., 45, -250));
    wall3->setColor(glm::vec3(0.8, 0.2, 0.5));
    wall3->setSpecularity(false);
    sceneObjects.push_back(wall3);

    Plane* wall4 = new Plane(
        glm::vec3(-30., -15, 0.5),
        glm::vec3(30., -15, 0.5),
        glm::vec3(30., 45, 0.5),
        glm::vec3(-30., 45, 0.5));
    wall4->setColor(glm::vec3(0, 0, 0));
    wall4->setSpecularity(false);
    wall4->setReflectivity(true);
    sceneObjects.push_back(wall4);

    Plane* ceiling = new Plane(
        glm::vec3(-30., 45, 0),
        glm::vec3(30., 45, 0),
        glm::vec3(30., 45, -250),
        glm::vec3(-30., 45, -250));
    ceiling->setColor(glm::vec3(0.5, 0.8, 0.2));
    sceneObjects.push_back(ceiling);

    // hollow spheres
    Sphere* hollowSphere = new Sphere(glm::vec3(-8.0, -2.5, -65.0), 3.5);
    hollowSphere->setColor(glm::vec3(0, 0.5, 0));
    hollowSphere->setTransparency(true, 0.7);
    hollowSphere->setSpecularity(false);
    hollowSphere->setReflectivity(true, 0.1);
    sceneObjects.push_back(hollowSphere);

    Sphere* hollowSphere2 = new Sphere(glm::vec3(-8.5, -4.0, -55.0), 2.0);
    hollowSphere2->setColor(glm::vec3(0.5, 0, 0));
    hollowSphere2->setTransparency(true, 0.7);
    hollowSphere2->setSpecularity(false);
    hollowSphere2->setReflectivity(true, 0.1);
    sceneObjects.push_back(hollowSphere2);

    // refractive sphere
    Sphere* refractiveSphere = new Sphere(glm::vec3(-7.5, 1.0, -80.0), 5.5);
    refractiveSphere->setColor(glm::vec3(0, 0.1, 0.3));
    refractiveSphere->setRefractivity(true, 1.0f, 1.5f);
    refractiveSphere->setReflectivity(true, 0.8);
    sceneObjects.push_back(refractiveSphere);

    // reflective sphere
    Sphere* reflectiveSphere = new Sphere(glm::vec3(-5.0, 8.0, -100.0), 8.0);
    reflectiveSphere->setColor(glm::vec3(0, 0.0, 0.0));
    reflectiveSphere->setReflectivity(true, 0.9);
    sceneObjects.push_back(reflectiveSphere);

    // texture mapped sphere
    Sphere* textureSphere = new Sphere(sphereCenter, 4.0);
    textureSphere->setColor(glm::vec3(1, 1, 1));
    textureSphere->setShininess(20);
    textureSphere->setSpecularity(true);
    sceneObjects.push_back(textureSphere);

    // cylinders
    Cylinder* cylinder = new Cylinder(glm::vec3(6.5, -15.0, -73.5), 7.0, 3.0);
    cylinder->setColor(glm::vec3(0.5, 0.2, 0.0));
    sceneObjects.push_back(cylinder);

    Cylinder* cylinder2 = new Cylinder(glm::vec3(6.5, -12.0, -73.5), 2.0, 5.0);
    cylinder2->setColor(glm::vec3(0.5, 0.2, 0.0));
    sceneObjects.push_back(cylinder2);
}


int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(30, 30);
    glutCreateWindow("COSC363: assignment 2 - ray tracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
