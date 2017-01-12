// TODO: separate depth variables for diffuse, specular and transparent

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <random>

using namespace std;

typedef glm::vec3 vec3;
typedef vec3 Color;
typedef vector<vector<vector<int> > > RgbImage;
typedef vector<vector<vec3> > Framebuffer;

const int WIDTH = 200;
const int HEIGHT = 200;
const int MAX_DEPTH = 3; // 0 = only point directly seen by camera
const float EPSILON = 0.00001f;
const float PI2 =(float) M_PI * 2.0f;
const float DIFFUSE_CONTRIBUTION = .80f;
float GAMMA_FACTOR = 0.8f;

// Refractive indecis: air = 1.0, glass = 1.5
const float REFRACTIVE_FACTOR_ENTER_GLASS = 1.0f / 1.5f;
const float REFRACTIVE_FACTOR_EXIT_GLASS = 1.5f / 1.0f;
const float CRITICAL_ANGLE = asin(REFRACTIVE_FACTOR_ENTER_GLASS);

default_random_engine generator;
uniform_real_distribution<float> distribution(0, 1);

float getRandom() {
  return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

struct Material {
  Color color;
  float diffuse;
  float specular;
  float transparency;
  float emmitance;

  Material(Color col,
           float dif = 1.0f,
           float spe = 0.0f,
           float opa = 0.0f,
           float emm = 0.0f)
      : color{col}, diffuse{dif}, specular{spe}, transparency{opa}, emmitance{emm} {
    // Prevent material from ADDING light (unless it is an emmiter)
    float sum = diffuse + specular + transparency;
    if (sum > 1.0f) {
      diffuse /= sum;
      specular /= sum;
      transparency /= sum;
    }
  }
};

struct Sphere {
  float radius;
  vec3 position;
  Material* material;

  Sphere(vec3 pos, float rad, Material* mat)
      : radius(rad), position(pos), material(mat) {}
};

struct Triangle {
  vec3* v1;
  vec3* v2;
  vec3* v3;
  vec3 normal;
  Material* material;

  Triangle(vec3* v1_, vec3* v2_, vec3* v3_, Material* mat) :
      v1{v1_}, v2{v2_}, v3{v3_}, material{mat} {
    CalculateNormal();
  }

  void CalculateNormal() {
    normal = glm::normalize(glm::cross((*v2) - (*v1), (*v3) - (*v2)));
  }
};

struct PointLight {
  vec3* position;
  float intensity = 1.0f;
  Color color;

  PointLight(vec3 *position, float intensity = 1.0f, Color color = Color(1.0f, 1.0f, 1.0f))
      : position(position), intensity(intensity), color(color) {}
};

struct Ray {
  vec3 origin;
  vec3 direction;

  Ray(vec3 ori, vec3 dir)
      : origin(ori), direction(glm::normalize(dir)) {}
};

void CreateCube(float w,
                float h,
                float d,
                vector<vec3>& va,
                vector<Triangle>& ta,
                vector<Triangle*>& ala,
                Material* mat,
                bool outwards_normals = true,
                vec3 center_position = vec3(0.0f, 0.0f, 0.0f)) {
  int v_idx = va.size();
  // Create Vertices
  {
    float dx = w / 2.0f;
    float dy = h / 2.0f;
    float dz = d / 2.0f;
    float cx = center_position.x;
    float cy = center_position.y;
    float cz = center_position.z;

    va.push_back(vec3(-dx + cx, -dy + cy,  dz + cz)); // left  bottom front
    va.push_back(vec3( dx + cx, -dy + cy,  dz + cz)); // right bottom front
    va.push_back(vec3( dx + cx,  dy + cy,  dz + cz)); // right top    front
    va.push_back(vec3(-dx + cx,  dy + cy,  dz + cz)); // left  top    front
    va.push_back(vec3( dx + cx, -dy + cy, -dz + cz)); // right bottom back
    va.push_back(vec3(-dx + cx, -dy + cy, -dz + cz)); // left  bottom back
    va.push_back(vec3(-dx + cx,  dy + cy, -dz + cz)); // left  top    back
    va.push_back(vec3( dx + cx,  dy + cy, -dz + cz)); // right top    back
  }
  // Create the triangles
  {
    if (outwards_normals) {
      ta.push_back(Triangle( &va[v_idx + 0], &va[v_idx + 1], &va[v_idx + 2], mat) ); // front bottom right
      ta.push_back(Triangle( &va[v_idx + 0], &va[v_idx + 2], &va[v_idx + 3], mat) ); // front top left
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 4], &va[v_idx + 7], mat) ); // right bottom right
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 7], &va[v_idx + 2], mat) ); // right top left
      ta.push_back(Triangle( &va[v_idx + 4], &va[v_idx + 5], &va[v_idx + 6], mat) ); // back bottom right
      ta.push_back(Triangle( &va[v_idx + 4], &va[v_idx + 6], &va[v_idx + 7], mat) ); // back top left
      ta.push_back(Triangle( &va[v_idx + 5], &va[v_idx + 0], &va[v_idx + 3], mat) ); // left bottom right
      ta.push_back(Triangle( &va[v_idx + 5], &va[v_idx + 3], &va[v_idx + 6], mat) ); // left top left
      ta.push_back(Triangle( &va[v_idx + 5], &va[v_idx + 4], &va[v_idx + 1], mat) ); // bottom bottom right
      ta.push_back(Triangle( &va[v_idx + 5], &va[v_idx + 1], &va[v_idx + 0], mat) ); // bottom top left
      ta.push_back(Triangle( &va[v_idx + 3], &va[v_idx + 2], &va[v_idx + 7], mat) ); // top bottom right
      ta.push_back(Triangle( &va[v_idx + 3], &va[v_idx + 7], &va[v_idx + 6], mat) ); // top top left
    } else {
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 0], &va[v_idx + 2], mat) ); // front bottom right
      ta.push_back(Triangle( &va[v_idx + 2], &va[v_idx + 0], &va[v_idx + 3], mat) ); // front top left
      ta.push_back(Triangle( &va[v_idx + 4], &va[v_idx + 1], &va[v_idx + 7], mat) ); // right bottom right
      ta.push_back(Triangle( &va[v_idx + 7], &va[v_idx + 1], &va[v_idx + 2], mat) ); // right top left
      ta.push_back(Triangle( &va[v_idx + 5], &va[v_idx + 4], &va[v_idx + 6], mat) ); // back bottom right
      ta.push_back(Triangle( &va[v_idx + 6], &va[v_idx + 4], &va[v_idx + 7], mat) ); // back top left
      ta.push_back(Triangle( &va[v_idx + 0], &va[v_idx + 5], &va[v_idx + 3], mat) ); // left bottom right
      ta.push_back(Triangle( &va[v_idx + 3], &va[v_idx + 5], &va[v_idx + 6], mat) ); // left top left
      ta.push_back(Triangle( &va[v_idx + 4], &va[v_idx + 5], &va[v_idx + 1], mat) ); // bottom bottom right
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 5], &va[v_idx + 0], mat) ); // bottom top left
      ta.push_back(Triangle( &va[v_idx + 2], &va[v_idx + 3], &va[v_idx + 7], mat) ); // top bottom right
      ta.push_back(Triangle( &va[v_idx + 7], &va[v_idx + 3], &va[v_idx + 6], mat) ); // top top left
    }
  }

  // If material is an emmiter, add it to the Area Light-source Array (ala)
  {
    if (mat->emmitance > 0.0f) {
      int t_idx = ta.size() - 12;
      for (int i = 0; i < 12; i++) {
        ala.push_back(&ta[t_idx + i]);
      }
    }
  }
}

void CreateFourTriangleQuad(float w,
                            float h,
                            vector<vec3> &va,
                            vector<Triangle> &ta,
                            vector<Triangle*> &ala,
                            Material* mat,
                            bool upwards_normal = true,
                            vec3 center_position = vec3(0.0f, 0.0f, 0.0f)) {
  float dx = w / 2.0f;
  float dz = h / 2.0f;
  float cx = center_position.x;
  float cy = center_position.y;
  float cz = center_position.z;

  int v_idx = va.size();

  va.push_back(vec3(cx - dx, cy, cz + dz)); // left  bottom
  va.push_back(vec3(cx + dx, cy, cz + dz)); // right bottom
  va.push_back(vec3(cx + dx, cy, cz - dz)); // right top
  va.push_back(vec3(cx - dx, cy, cz - dz)); // left  top
  va.push_back(vec3(cx     , cy, cz     )); // center

  // Create the triangles
  {
    if (upwards_normal) { //TODO byt plats på y & z?
      ta.push_back(Triangle( &va[v_idx + 0], &va[v_idx + 1], &va[v_idx + 4], mat) ); // front bottom right
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 2], &va[v_idx + 4], mat) ); // front top left
      ta.push_back(Triangle( &va[v_idx + 2], &va[v_idx + 3], &va[v_idx + 4], mat) ); // right bottom right
      ta.push_back(Triangle( &va[v_idx + 3], &va[v_idx + 0], &va[v_idx + 4], mat) ); // right top left
    } else {
      ta.push_back(Triangle( &va[v_idx + 1], &va[v_idx + 0], &va[v_idx + 4], mat) ); // front bottom right
      ta.push_back(Triangle( &va[v_idx + 2], &va[v_idx + 1], &va[v_idx + 4], mat) ); // front top left
      ta.push_back(Triangle( &va[v_idx + 3], &va[v_idx + 2], &va[v_idx + 4], mat) ); // right bottom right
      ta.push_back(Triangle( &va[v_idx + 0], &va[v_idx + 3], &va[v_idx + 4], mat) ); // right top left
    }
  }

  if (mat->emmitance > 0.0f) {
    int t_idx = ta.size() - 4;
    for (int i = 0; i < 4; i++) {
        ala.push_back(&ta[t_idx + i]);
    }
  }
}

void CreateScene(vector<vec3> &va,
                 vector<Triangle> &ta,
                 vector<Material> &ma,
                 vector<PointLight> &pla,
                 vector<Triangle*> &ala) {
  // Default materials (whiteish, blackish, redish, greenish, blueish)
  {
    ma.push_back(Material(vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f));       // 0 WHITE
    ma.push_back(Material(vec3(0.1f, 0.1f, 0.1f), 1.0f, 0.0f, 0.0f));       // 1 BLACKISH
    ma.push_back(Material(vec3(1.0f, 0.5f, 0.5f), 1.0f, 0.0f, 0.0f));       // 2 REDISH
    ma.push_back(Material(vec3(0.5f, 1.0f, 0.5f), 1.0f, 0.0f, 0.0f));       // 3 GREENISH
    ma.push_back(Material(vec3(0.5f, 0.5f, 1.0f), 1.0f, 0.0f, 0.0f));       // 4 BLUEISH
    ma.push_back(Material(vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.1f)); // 5 WHITE EMMITER
    ma.push_back(Material(vec3(1.0f, 1.0f, 1.0f), 0.0f, 1.0f, 0.0f));       // 6 PERFECT MIRROR
    ma.push_back(Material(vec3(1.0f, 0.0f, 0.0f), 0.6f, 0.4f, 0.0f));       // 7 RUBY
    ma.push_back(Material(vec3(0.0f, 1.0f, 0.0f), 0.4f, 0.6f, 0.0f));       // 8 EMERALD
    ma.push_back(Material(vec3(0.0f, 0.0f, 1.0f), 0.6f, 0.4f, 0.0f));       // 9 SAPPHIRE
  }
  // Create the Cornell Box
  {
    CreateCube(10.0f, 10.0f, 10.0f, va, ta, ala, &ma[0], false, vec3(0.0f, 0.0f, 0.0f));
    // ta[0].material = &ma[6]; // FRONT
    // ta[1].material = &ma[6];
    ta[2].material = &ma[4]; // RIGHT SIDE
    ta[3].material = &ma[4];
    ta[4].material = &ma[3]; // BACK
    ta[5].material = &ma[3];
    ta[6].material = &ma[2]; // LEFT
    ta[7].material = &ma[2];
    // ta[8].material = &ma[0]; // BOTTOM
    // ta[9].material = &ma[0];
    // ta[10].material = &ma[6]; // TOP
    // ta[11].material = &ma[6];
  }

  //Create additional geometry
  {
    CreateCube(1.25f, 1.25f, 1.25f, va, ta, ala, &ma[9], true, vec3(-2.0f, -1.8f, -1.0f));
    CreateCube(1.4f, 1.4f, 1.4f, va, ta, ala, &ma[7], true, vec3(2.0f, -1.6f, -1.0f));
    // CreateCube(2.0f, 0.1f, 2.0f, va, ta, ala, &ma[5], true, vec3(0.0f, 4.05f, 0.0f));
    CreateFourTriangleQuad(2.0f, 2.0f, va, ta, ala, &ma[5], false, vec3(0.0f, 4.95f, 0.0f));
  }
  //Create Point Lights
  {
    // int v_idx = va.size();
    // va.push_back(vec3(0.0f, 4.0f, 0.0f));
    // pla.push_back(PointLight( &va[v_idx], 0.8f, Color(1.0f, 1.0f, 1.0f)));
  }
}

void SaveImage(const char* img_name, Framebuffer &image) {
  float gamma_factor_inv = 1.0f / GAMMA_FACTOR;
  FILE* fp = fopen(img_name, "wb"); /* b - binary mode */
  (void)fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
  for (int i = WIDTH - 1; i >= 0; i--) {
    for (int j = 0 ; j < HEIGHT; j++) {
      float r = image[j][i][0];
      float g = image[j][i][1];
      float b = image[j][i][2];
      static unsigned char color[3];
      // color[0] = (int)(255 * (r < 0 ? 0 : r > 1 ? 1 : r));  // red
      // color[1] = (int)(255 * (g < 0 ? 0 : g > 1 ? 1 : g));  // green
      // color[2] = (int)(255 * (b < 0 ? 0 : b > 1 ? 1 : b));  // blue
      color[0] = (int)(255 * pow(r < 0.0f ? 0.0f : r > 1.0f ? 1.0f : r, gamma_factor_inv));  // red
      color[1] = (int)(255 * pow(g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g, gamma_factor_inv));  // green
      color[2] = (int)(255 * pow(b < 0.0f ? 0.0f : b > 1.0f ? 1.0f : b, gamma_factor_inv));  // blue
      (void)fwrite(color, 1, 3, fp);
    }
  }
}

void ClearColorBuffer(Color clear_color, Framebuffer &frame_buffer) {
  for(int x = 0; x < WIDTH; x++) {
    for(int y = 0; y < HEIGHT; y++) {
      frame_buffer[x][y] = clear_color;
    }
  }
}

// TODO     
// Corresponding MöllerTrumbore For Sphere     

bool MoellerTrumbore(Ray &ray, Triangle &triangle, vec3 &collision_point) {
  vec3 ps = ray.origin;
  vec3 D = ray.direction;
  vec3 v0 = *(triangle.v1);
  vec3 v1 = *(triangle.v2);
  vec3 v2 = *(triangle.v3);

  // According to the Möller Trumbore intersection algorithm
  vec3 T = ps - v0;
  vec3 E1 = v1 - v0;
  vec3 E2 = v2 - v0;
  vec3 P = glm::cross(D, E2);

  float det = glm::dot(P, E1);

  vec3 Q = glm::cross(T, E1);
  float t = glm::dot(Q,E2) / det;
  float u = glm::dot(P, T) / det;
  float v = glm::dot(Q, D) / det;

  if(u >= 0.0f && v >= 0.0f && u+v <= 1.0f && t > 0.0f) {
    collision_point = (1.0f - u - v) * v0 + u * v1 + v * v2;
    return true;
  }
  return false;
}

float ShadowRay(Ray& ray,
                vector<Triangle>& ta,
                float z_buffer,
                Triangle* area_light_triangle = nullptr) {
  float o_light_factor = 1.0f;
  int t_length = ta.size();
  for (auto& triangle : ta) {
  // for (int i = 0; i < t_length; i++) {
    vec3 temp_collision_point;
    if (&triangle != &(*area_light_triangle) &&
        MoellerTrumbore(ray, triangle, temp_collision_point)) {
      // if (MoellerTrumbore(ray, ta[i], temp_collision_point)) {
      float z_temp = glm::length(ray.origin - temp_collision_point);
      if (z_temp < z_buffer) {
        o_light_factor *= triangle.material->transparency;
        // o_light_factor *= ta[i].material->transparency;
        if (o_light_factor < 0.00001f) {
          return 0.0f;
        }
      }
    }
  }
  return o_light_factor;
}

bool GetClosestIntersectionPoint(Ray &ray,
                                 vector<Triangle> &ta,
                                 float &z_buffer,
                                 Triangle* &triangle_pointer,
                                 vec3 &collision_point) {
  // cout << "Closest Intersection Point" << endl;
  bool has_found_intersection_point = false;
  vec3 ray_origin = ray.origin;
  int t_length = ta.size();
  // for (auto& triangle : ta) {
  for (int i = 0; i < t_length; i++) {
    vec3 temp_collision_point;
    // if (MoellerTrumbore(ray, triangle, temp_collision_point)) {
    // cout << "Pre-Möller" << endl;
    if (MoellerTrumbore(ray, ta[i], temp_collision_point)) {
      float z_temp = glm::length(ray_origin - temp_collision_point);
      if (z_temp < z_buffer) {
        // cout << "\t\tEven found a closer point!" << endl;
        z_buffer = z_temp;
        has_found_intersection_point = true;
        // triangle_pointer = &triangle;
        triangle_pointer = &ta[i];
        collision_point = temp_collision_point;
      }
      // cout << "Möller EXIT" << endl;
    }
  }
   // cout << "\tClosest Intersection Point END" << endl;
  return has_found_intersection_point;
}

// Forward declaration, just for now.. should probably later be part of a Raytrace class?
Color Raytrace(Ray& ray,
               vector<Triangle> &ta,
               vector<PointLight> &pla,
               vector<Triangle*> &ala,
               unsigned int depth);

Color HandleDirectIllumination(vector<PointLight> &pla,
                               vector<Triangle*> &ala,
                               vector<Triangle> &ta,
                               Triangle* triangle,
                               vec3 &collision_point) {
  Color radiance = Color(0.0f, 0.0f, 0.0f);
  // TODO: create separate function that take in the pos, intensity & color?
  for(auto &point_light : pla) {
    vec3 relative_vec = *point_light.position - collision_point;
    vec3 light_direction = glm::normalize(relative_vec);

    float visibilty_factor = glm::dot(triangle->normal, light_direction);
    if( visibilty_factor < 0.0f) {
      continue;
    }

    Ray ray(collision_point + 0.00001f * triangle->normal, light_direction);
    float shadow_value = ShadowRay(ray, ta, glm::length(relative_vec));

    if (shadow_value < 0.00001f) {
      continue;
    }
    // TODO: falloff-radius of lights?
    radiance += point_light.intensity * point_light.color *
                    visibilty_factor * shadow_value;
  }

  for(auto &area_light_triangle : ala) {
    // Randomize a point on the triangle using barycentric coordinates
    // float u = distribution(generator);
    // float v = distribution(generator);
    float u = getRandom();
    float v = getRandom();
    float w = getRandom();
    float sum = u + v + w;
    // if (sum > 1.0f) { // todo: this might be biased?
      u /= sum;
      v /= sum;
      w /= sum;
    // }

    vec3 alp = *(area_light_triangle->v1) * w +
               *(area_light_triangle->v2) * u +
               *(area_light_triangle->v3) * v;  // area-light point

    vec3 relative_vec = alp - collision_point;
    vec3 light_direction = glm::normalize(relative_vec);

    float visibilty_factor = glm::dot(triangle->normal, light_direction);
    if( visibilty_factor < 0.0f) {
      continue; //visibilty_factor = 0.0f;
    }

    Ray ray(collision_point + 0.00001f * triangle->normal, light_direction);
    float shadow_value =
        ShadowRay(ray, ta, glm::length(relative_vec), area_light_triangle);

    if (shadow_value < 0.00001f) {
      continue;
    }
    // TODO: falloff-radius of lights?
    radiance += area_light_triangle->material->emmitance *
                area_light_triangle->material->color * visibilty_factor /*
                shadow_value*/;
  }
  return radiance;
}

// TODO: PointLights aren't doing anything here but is needed in the Raytrace function.. raytracer should probably be a class
Color HandleDiffuse(int depth,
                    vector<Triangle> &ta,
                    Triangle* triangle,
                    vec3& collision_point,
                    vector<PointLight> &pla,
                    vector<Triangle*> &ala) {
  // cout << "\tHandling diffuse case!" << endl;
  vec3 w = triangle->normal;
  // Generalized formula for finding tangent u given a unit length normal vector w
  vec3 u = glm::normalize(glm::cross(fabs(w.x) > 0.1f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f), w));
  vec3 v = glm::cross(w, u);

  // Random direction d within hemisphere
  // Random angle
  // float r1 = PI2 * distribution(generator);
  float r1 = PI2 * getRandom();
  // Random distance from center
  // float r2 = distribution(generator);
  float r2 = getRandom();
  float r2s = sqrtf(r2);
  vec3 d = glm::normalize(u * (float)cos(r1) * r2s + v*(float)sin(r1) * r2s + w * sqrtf(1.0f - r2));

  vec3 reflection_point_origin = collision_point + w * 0.00001f;
  Ray ray = Ray(reflection_point_origin, d);
  // cout << "\t\tDiffuse DONE!!" << endl;
  return DIFFUSE_CONTRIBUTION * triangle->material->diffuse *
         Raytrace(ray, ta, pla, ala, ++depth);
}

Color HandleSpecular(Ray& ray,
                     int depth,
                     vector<Triangle>& ta,
                     Triangle* triangle,
                     vec3& collision_point,
                     vector<PointLight>& pla,
                     vector<Triangle*>& ala/*,
                     reverse_normal = false*/) {
  // vec3 n = reverse_normal ? -triangle->normal, triangle->normal;
  vec3 n = triangle->normal;
  vec3 d = ray.direction;
  if (glm::dot(n,d) > 0) {
    n = -n;
  }

  vec3 reflection_point_origin = collision_point + n * 0.00001f;
  vec3 reflection_direction = d - 2 * (glm::dot(d, n))*n;
  Ray reflection_ray = Ray(reflection_point_origin, reflection_direction);
  return triangle->material->specular *
         Raytrace(reflection_ray, ta, pla, ala, ++depth);
}

Color HandleRefraction(Ray& ray,
                       int depth,
                       vector<Triangle>& ta,
                       Triangle* triangle,
                       vec3& collision_point,
                       vector<PointLight>& pla,
                       vector<Triangle*>& ala/*,
                       reverse_normal = false*/) {
  vec3 n = triangle->normal;
  vec3 I = ray.direction;
  // assert(std::abs(1.0f - (float)glm::length(p.get_normal())) < EPSILON);
  // assert(std::abs(1.0f - (float) glm::length(I)) < EPSILON);
  float I_dot_n = glm::dot(n,I);
  float refractive_factor;

  // From outside going in
  if (I_dot_n < 0.0f) {
    refractive_factor = REFRACTIVE_FACTOR_ENTER_GLASS;
  } else { // From inside going out
    refractive_factor = REFRACTIVE_FACTOR_EXIT_GLASS;
    n = -n;
    // I_dot_n = glm::dot(n,I); // TODO: this might cause errors.. try comment out if weird behaviour
    float alpha = acos(I_dot_n);
    if ( alpha > CRITICAL_ANGLE ) { // total inner reflection occurs (only reflection term)
      return Color(0.0f, 0.0f, 0.0f);
    }
  }

  vec3 T = refractive_factor * I - n * (refractive_factor * I_dot_n +
      sqrtf(1.f - refractive_factor * refractive_factor * (1.f - I_dot_n * I_dot_n)));
  vec3 refraction_point_origin = collision_point + n * EPSILON; //TODO: Changed to addition from subtraction.. wrong or correct?
  Ray refraction_ray = Ray(refraction_point_origin, T);
  return triangle->material->transparency * Raytrace(refraction_ray, ta, pla, ala, ++depth);
}

Color Raytrace(Ray& ray,
               vector<Triangle>& ta,
               vector<PointLight>& pla,
               vector<Triangle*>& ala,
               unsigned int depth) {
  Color radiance = Color(0.0f, 0.0f, 0.0f);

  // BREAK RECURISVENESS IF MAX DEPTH HAS BEEN REACHED
  if (depth > MAX_DEPTH) {
    return radiance;
  }

  // string temp;
  // for (int i = 0; i <= depth; i++) {
  //   temp += "\t";
  // }
  // cout << temp << "Raytrace at depth: " << depth << endl;

  float z_buffer = FLT_MAX;
  Triangle* triangle;
  vec3 collision_point;

  // Get the ray intersection with the nearest surface
  if (GetClosestIntersectionPoint(ray, ta, z_buffer, triangle, collision_point)) {
    // Self emmitance - Required for area light sources
    float diffuse = triangle->material->diffuse;
    float specular = triangle->material->specular;
    float transparency = triangle->material->transparency;
    float emmitance = triangle->material->emmitance;

    Color self_emmitance = Color(0.0f, 0.0f, 0.0f);
    if(emmitance > 0.0f) {
      self_emmitance = (1 + emmitance) * triangle->material->color;
    }

    // Direct Illumination - Required for point lights
    radiance += HandleDirectIllumination(pla, ala, ta, triangle, collision_point);

    if (diffuse > 0.0f) {
      radiance += HandleDiffuse(depth, ta, triangle, collision_point, pla, ala) *  triangle->material->color;
    }

    // if (transparency > 0.0f) {
      // If outside going into object
      // if (glm::dot(triangle.normal, ray.direction) < 0.0f) {
        if (specular > 0.0f) {
          radiance += HandleSpecular(ray, depth, ta, triangle, collision_point, pla, ala);
        }
        if (transparency > 0.0f) {
          radiance += HandleRefraction(ray, depth, ta, triangle, collision_point, pla, ala);
        }
      // } else { // reversed normal, we are inside an object

      // }
    // }
    return self_emmitance + radiance; //* triangle->material->color;
  }
                                         
  //TODO: SAME THING FOR SPHERES         
                                         
  cerr << "\nLigg här och gnag... " << endl;
  return radiance;
}

void Render(vec3 cam_pos,
            vector<Triangle> &ta,
            vector<PointLight> &pla,
            vector<Triangle*> &ala,
            Framebuffer &frame_buffer,
            float delta,
            float pixel_center_minimum,
            int spp = 1) {
  float delta2 = delta - (delta / 2.0f);
  cout << "Rendering!" << endl;
  for (int i = 0; i < WIDTH; i++) {
    // fprintf(stderr, "\r\tProgress:  %1.2f%%", 100. * i / (WIDTH - 1));
    for (int j = 0; j < HEIGHT; j++) {
      Color temp_color = Color(0.0f, 0.0f, 0.0f);
      for (int s = 0; s < spp; s++) {
        // cout << "Rendering pixel (" << i << ", " << j << ")" << endl;
        float random_x = distribution(generator) * delta2;
        float random_y = distribution(generator) * delta2;

        vec3 pixel_random_point =
            vec3(i * delta + pixel_center_minimum + random_x,
                 j * delta + pixel_center_minimum + random_y, 4.0f);
        Ray ray(pixel_random_point, pixel_random_point - cam_pos);
        temp_color = temp_color + Raytrace(ray, ta, pla, ala, 0);
        // cout << "Rendering pixel (" << i << ", " << j << ") DONE!" << endl;
      }
      frame_buffer[i][j] = (temp_color / (float)spp);
    }
    fprintf(stderr, "\r\tRendering progress:  %1.2f%%", 100.*i/(WIDTH-1));
  }
  cout << endl;
}

int main() {
  cout << "GI-Ray to the rescue" << endl;

  // Create camera
  vec3 camera_pos = vec3(0.0f, 0.0f, 4.95f);
  vec3 camera_plane[4];
  camera_plane[0] = vec3(-1.0f,-1.0f, 4.0f);
  camera_plane[1] = vec3( 1.0f,-1.0f, 4.0f);
  camera_plane[2] = vec3( 1.0f, 1.0f, 4.0f);
  camera_plane[3] = vec3(-1.0f, 1.0f, 4.0f);

  float delta = (camera_plane[1].x - camera_plane[0].x)/WIDTH; // assumes square pixels
  float pixel_center_minimum = camera_plane[0].x + delta/2;

  Framebuffer frame_buffer(WIDTH, vector<vec3>(HEIGHT));

  ClearColorBuffer(Color(1.0f, 0.2f, 0.9f), frame_buffer);

  vector<vec3>        vertex_array;
  vector<Material>    material_array;
  vector<Triangle>    triangle_array;
  vector<PointLight>  point_light_array;
  vector<Triangle*>   area_light_array;

  vertex_array.reserve(100);
  material_array.reserve(100);
  triangle_array.reserve(100);
  point_light_array.reserve(100);
  area_light_array.reserve(100);

  cout << "Creating the scene..." << endl;
  CreateScene(vertex_array, triangle_array, material_array, point_light_array,
              area_light_array);

  Render(camera_pos, triangle_array, point_light_array, area_light_array,
        frame_buffer, delta, pixel_center_minimum, 20);

  float newGamma = 1.0f;
  string test;
  while(true) {
    string filename = "bananskal.ppm";
    cout << "Render DONE!\nWriting to file: " << filename << endl;
    SaveImage(filename.c_str(), frame_buffer);
    cout << "Enter new gamma value: " << endl;
    cin >> test;
    newGamma = stof(test);
    if (newGamma <= 0) {
      break;
    }
    GAMMA_FACTOR = newGamma;
  }

  cout << "\nGI-Ray finished without any problems" << endl;
  cout << "\nEXITING GI-RAY..." << endl;

  return 0;
}