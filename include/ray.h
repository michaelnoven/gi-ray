#ifndef RAY_H
#define RAY_H

#include "commons.h"
#include "intersection_point.h"

class Ray
{
private:
  Vertex v_start_;
  Vertex v_end_;
  ColorDbl color_;
  IntersectionPoint intersection_point_;

public:
  Ray() {}
  Ray(Vertex v_start, Vertex v_end) : v_start_(v_start), v_end_(v_end) {}

  //TODO: implement GetTriangle() ?
  Vertex start() { return v_start_; }
  Vertex end() { return v_end_; }
  ColorDbl get_color() { return color_;}

  void set_color(ColorDbl color) { color_ = color; }
  void set_intersection_point(IntersectionPoint p) { intersection_point_ = p; }

};

#endif // RAY_H
