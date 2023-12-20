#include "render.h"
#include "math.h"
#include "float.h"

sphere sphere_create(vec3 center, float radius, material mat)
{
    sphere res;
    res.center = center;
    res.radius = radius;
    res.mat = mat;
    return res;
}

light light_create(vec3 position, float intensity)
{
    light res;
    res.position = position;
    res.intensity = intensity;
    return res;
}

material material_create(vec3 color, vec3 albedo, float specular_exponent)
{
    material mat;
    mat.diffuse_color = color;
    mat.albedo = albedo;
    mat.specular_exponent = specular_exponent;
    return mat;
}

vec3 reflect(vec3 I, vec3 N)
{
    return vector_diff(I,
                       vector_multiplication(
                           vector_multiplication(N, 2.f),
                           vector_scalar_product(I, N)));
}

int sphere_ray_intersect(sphere s, vec3 orig, vec3 dir, float *dist)
{
    vec3 diff = vector_diff(s.center, orig);

    float tca = vector_scalar_product(diff, dir);
    float d2 = vector_scalar_product(diff, diff) - (tca * tca);

    if (d2 > s.radius * s.radius)
        return 0;

    float thc = sqrtf(s.radius * s.radius - d2);

    *dist = tca - thc;

    if (*dist < 0)
        *dist = tca + thc;

    if (*dist < 0)
        return 0;

    return 1;
}

int scene_intersect(vec3 orig, vec3 dir,
                    sphere *spheres, size_t spheres_len,
                    vec3 *hit, vec3 *normal, material *material)
{
    float spheres_dist = FLT_MAX;

    for (size_t i = 0; i < spheres_len; ++i)
    {
        float dist_i;
        if (sphere_ray_intersect(spheres[i], orig, dir, &dist_i) && dist_i < spheres_dist)
        {
            spheres_dist = dist_i;
            *hit = vector_addition(orig, vector_multiplication(dir, dist_i));
            *normal = vector_normalize(vector_diff(*hit, spheres[i].center));
            *material = spheres[i].mat;
        }
    }

    return spheres_dist < 1000;
}

vec3 cast_ray(vec3 orig, vec3 dir, vec3 background_color,
              sphere *spheres, size_t spheres_len,
              light *lights, size_t lights_len, size_t depth)
{
    vec3 point, normal;
    material mat;

    if (depth > 4 || !scene_intersect(orig, dir, spheres, spheres_len, &point, &normal, &mat))
    {
        return background_color;
    }

    vec3 reflect_dir = vector_normalize(reflect(dir, normal));
    vec3 reflect_orig = vector_scalar_product(reflect_dir, normal) < 0.f ? vector_diff(point, vector_multiplication(normal, 1e-3))
                                                                         : vector_addition(point, vector_multiplication(normal, 1e-3));
    vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, background_color,
                                  spheres, spheres_len, lights, lights_len, depth + 1);

    float diffuse_light_intensity = 0.f, specular_light_intensity = 0.f;

    for (size_t i = 0; i < lights_len; ++i)
    {
        vec3 light_dir = vector_normalize(vector_diff(lights[i].position, point));

        float light_distanse = vector_norm(vector_diff(lights[i].position, point));

        vec3 shadow_orig = vector_scalar_product(light_dir, normal) < 0.f ? vector_diff(point, vector_multiplication(normal, 1e-3))
                                                                          : vector_addition(point, vector_multiplication(normal, 1e-3));

        vec3 shadow_point, shadow_normal;
        material tmp;
        if (scene_intersect(shadow_orig, light_dir, spheres, spheres_len, &shadow_point, &shadow_normal, &tmp) &&
            vector_norm(vector_diff(shadow_point, shadow_orig)) < light_distanse)
            continue;

        float angle_light_to_normal = vector_scalar_product(light_dir, normal);
        diffuse_light_intensity += lights[i].intensity * MAX(angle_light_to_normal, 0.f);
        float reflect_angle = vector_scalar_product(reflect(light_dir, normal), dir);
        specular_light_intensity += powf(MAX(reflect_angle, 0.f), mat.specular_exponent) * lights[i].intensity;
    }

    return vector_addition(vector_addition(
                               vector_multiplication(
                                   vector_multiplication(mat.diffuse_color, diffuse_light_intensity), mat.albedo.x),
                               vector_multiplication(
                                   vector_multiplication(vector_create(1., 1., 1.), specular_light_intensity),
                                   mat.albedo.y)),
                           vector_multiplication(reflect_color, mat.albedo.z));
}
