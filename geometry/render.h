#ifndef SPHERE_H
#define SPHERE_H
#include "vector.h"

#ifndef __cplusplus
typedef __SIZE_TYPE__ size_t;
#endif

typedef struct material
{
    vec3 diffuse_color;
    vec3 albedo;
    float specular_exponent;
} material;

typedef struct sphere
{
    float radius;
    vec3 center;
    material mat;
} sphere;

typedef struct light
{
    vec3 position;
    float intensity;
} light;

#ifdef __cplusplus
extern "C"
{
#endif
    // Macros
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

    sphere sphere_create(vec3 center, float radius, material mat);

    light light_create(vec3 position, float intensity);

    material material_create(vec3 color, vec3 albedo, float specular_exponent);

    vec3 reflect(vec3 I, vec3 N);

    int sphere_ray_intersect(sphere s, vec3 orig, vec3 dir, float *dist);

    int scene_intersect(vec3 orig, vec3 dir, sphere *spheres, size_t spheres_len, vec3 *hit, vec3 *normal, material *material);

    vec3 cast_ray(vec3 orig, vec3 dir, vec3 background_color, sphere *spheres, size_t spheres_len, light *lights, size_t lights_len, size_t depth);
#ifdef __cplusplus
}
#endif

#endif