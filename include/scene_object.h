#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "commons.h"

class Ray;

class SceneObject {
public:
  virtual ~SceneObject() = default;
  virtual float RayIntersection(Ray& ray) = 0;

protected:
  //Vertex center_pos_; //TODO: calculate this in constructor
};

#endif // SCENE_OBJECT_H