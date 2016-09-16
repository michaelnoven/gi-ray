#include "scene.h"
#include "commons.h"
#include "triangle_custom_shape.h"
#include "tetrahedron.h"

Scene::Scene() {
  InitRoom();
  //InitObjects();
}

void Scene::InitRoom() {
  // Floor vertices
  Vertex vfC = Vertex( 5,  0, -5); // center vertex on floor
  Vertex vf1 = Vertex(-3,  0, -5);
  Vertex vf2 = Vertex( 0, -6, -5);
  Vertex vf3 = Vertex(10, -6, -5);
  Vertex vf4 = Vertex(13,  0, -5);
  Vertex vf5 = Vertex(10,  6, -5);
  Vertex vf6 = Vertex( 0,  6, -5);

  // Ceiling vertices
  Vertex vcC = Vertex( 5,  0,  5); // center vertex on ceiling
  Vertex vc1 = Vertex(-3,  0,  5);
  Vertex vc2 = Vertex( 0, -6,  5);
  Vertex vc3 = Vertex(10, -6,  5);
  Vertex vc4 = Vertex(13,  0,  5);
  Vertex vc5 = Vertex(10,  6,  5);
  Vertex vc6 = Vertex( 0,  6,  5);

  // Colors
  glm::vec3  color_floor   = COLOR_WHITE;
  glm::vec3  color_ceiling = COLOR_BLACK;
  glm::vec3  color_wall1   = COLOR_RED;
  glm::vec3  color_wall2   = COLOR_GREEN;
  glm::vec3  color_wall3   = COLOR_BLUE;
  glm::vec3  color_wall4   = COLOR_RED;
  glm::vec3  color_wall5   = COLOR_GREEN;
  glm::vec3  color_wall6   = COLOR_BLUE;

  std::vector<Triangle> triangle_list;

  // Floor
  triangle_list.push_back(Triangle(vfC, vf6, vf1, color_floor));
  triangle_list.push_back(Triangle(vfC, vf1, vf2, color_floor));
  triangle_list.push_back(Triangle(vfC, vf2, vf3, color_floor));
  triangle_list.push_back(Triangle(vfC, vf3, vf4, color_floor));
  triangle_list.push_back(Triangle(vfC, vf4, vf5, color_floor));
  triangle_list.push_back(Triangle(vfC, vf5, vf6, color_floor));

  // Ceiling
  triangle_list.push_back(Triangle(vcC, vc1, vc6, color_ceiling));
  triangle_list.push_back(Triangle(vcC, vc2, vc1, color_ceiling));
  triangle_list.push_back(Triangle(vcC, vc3, vc2, color_ceiling));
  triangle_list.push_back(Triangle(vcC, vc4, vc3, color_ceiling));
  triangle_list.push_back(Triangle(vcC, vc5, vc4, color_ceiling));
  triangle_list.push_back(Triangle(vcC, vc6, vc5, color_ceiling));

  /* Counter-clockwise order, starting with front */

  // Wall1
  triangle_list.push_back(Triangle(vf2, vc2, vf3, color_wall1));
  triangle_list.push_back(Triangle(vf3, vc2, vc3, color_wall1));

  // Wall 2
  triangle_list.push_back(Triangle(vf3, vc3, vf4, color_wall2));
  triangle_list.push_back(Triangle(vf4, vc3, vc4, color_wall2));

  // Wall 3
  triangle_list.push_back(Triangle(vf4, vc4, vf5, color_wall3));
  triangle_list.push_back(Triangle(vc4, vc5, vf5, color_wall3));

  // Wall 4
  triangle_list.push_back(Triangle(vc5, vf6, vf5, color_wall4));
  triangle_list.push_back(Triangle(vc5, vc6, vf6, color_wall4));

  // Wall 5
  triangle_list.push_back(Triangle(vc6, vc1, vf6, color_wall5));
  triangle_list.push_back(Triangle(vc1, vf1, vf6, color_wall5));

  // Wall 6
  triangle_list.push_back(Triangle(vf2, vf1, vc2, color_wall6));
  triangle_list.push_back(Triangle(vf1, vc1, vc2, color_wall6));

  scene_objects_.push_back(
      std::make_unique<TriangleCustomShape>(triangle_list));
}
