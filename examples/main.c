#include "vector.h"
#include "render.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "float.h"
#include "thread_pool.h"
// Global variables
const size_t width = 3840;
const size_t height = 2160;
const size_t width_parts = 4;
const size_t height_parts = 2;
const size_t step_width = width / width_parts;
const size_t step_height = height / height_parts;
const int fov = M_PI / 2;
vec3 *framebuffer;

// Structs
typedef struct render_thread_args
{
    size_t width_part_idx;
    size_t height_part_idx;
    sphere *spheres;
    size_t spheres_len;
    light *lights;
    size_t lights_len;
    const vec3 *orig;
} render_thread_args;

// Functions
void render_thread(void *args)
{
    render_thread_args *render_args = (render_thread_args *)args;

    size_t depth = 0;

    size_t begin_width = step_width * render_args->width_part_idx;
    size_t begin_height = step_height * render_args->height_part_idx;

    size_t end_width = begin_width + step_width;
    size_t end_height = begin_height + step_height;

    for (size_t height_idx = begin_height; height_idx < end_height; ++height_idx)
    {
        for (size_t width_idx = begin_width; width_idx < end_width; ++width_idx)
        {
            float x = (2 * (width_idx + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (height_idx + 0.5) / (float)height - 1) * tan(fov / 2.);
            vec3 dir_not_normal;
            dir_not_normal.x = x;
            dir_not_normal.y = y;
            dir_not_normal.z = -1;
            vec3 dir = vector_normalize(dir_not_normal);

            framebuffer[width_idx + height_idx * width] =
                cast_ray(
                    *render_args->orig,
                    dir,
                    vector_create(0.5f, 0.5f, 0.5f),
                    render_args->spheres, render_args->spheres_len,
                    render_args->lights, render_args->lights_len, depth);
        }
    }

    free(render_args);
}

void render(thread_pool_t pool, sphere *spheres, size_t spheres_len, light *lights, size_t lights_len)
{
    const vec3 camera_pos = vector_create(0.f, 0.f, 0.f);

    framebuffer = (vec3 *)malloc(width * height * sizeof(vec3));
    if (framebuffer == NULL)
    {
        printf("Error allocate memory for framebuffer");
        return;
    }

    for (size_t i = 0; i < width_parts; ++i)
    {
        for (size_t j = 0; j < height_parts; ++j)
        {
            render_thread_args *args = (render_thread_args *)malloc(sizeof(render_thread_args));
            if (args == NULL)
            {
                printf("Error allocate memory for render thread args");
                return;
            }

            args->width_part_idx = i;
            args->height_part_idx = j;
            args->orig = &camera_pos;
            args->spheres = spheres;
            args->spheres_len = spheres_len;
            args->lights = lights;
            args->lights_len = lights_len;

            add_task_thread_pool(pool, render_thread, args);
        }
    }

    wait_thread_pool(pool);
    FILE *out = fopen("out.tga", "w");
    char header[18] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       (char)(width % 256), (char)(width / 256),
                       (char)(height % 256), (char)(height / 256),
                       (char)(3 * 8), 0x20};
    fwrite(&header, 18, 1, out);
    // FILE *out = fopen("out.ppm", "w");
    // fprintf(out, "P6 %d %d 255 ", width, height);

    for (size_t i = 0; i < height * width; ++i)
    {
        vec3 *frame = &framebuffer[i];
        float max = MAX(frame->x, MAX(frame->y, frame->z));
        if (max > 1)
            *frame = vector_multiplication(*frame, (1. / max));

        fprintf(out, "%c%c%c",
                (char)(255.f * (MAX(0.f, MIN(1.f, frame->x)))),
                (char)(255.f * (MAX(0.f, MIN(1.f, frame->y)))),
                (char)(255.f * (MAX(0.f, MIN(1.f, frame->z)))));
    }
    fclose(out);
    free(framebuffer);
}

int main()
{
    material green = material_create(vector_create(0.f, 0.5f, 0.f), vector_create(0.6f, 0.3f, 0.1f), 50.);
    material white = material_create(vector_create(1.f, 1.f, 1.f), vector_create(0.6f, 0.6f, 0.f), 20.);
    material mirror = material_create(vector_create(1.f, 1.f, 1.f), vector_create(0.0f, 10.f, 0.8f), 1425.);

    sphere *spheres = (sphere *)malloc(4 * sizeof(sphere));
    if (spheres == NULL)
    {
        printf("Error allocate memory for spheres");
        return 0;
    }
    spheres[0] = sphere_create(vector_create(-5.f, -1.f, -12.f), 2.f, mirror);
    spheres[1] = sphere_create(vector_create(1.5f, -0.5f, -18.f), 2.f, white);
    spheres[2] = sphere_create(vector_create(7.f, 5.f, -18.f), 2.f, mirror);
    spheres[3] = sphere_create(vector_create(-6.f, 3.f, -10.f), 3.f, green);

    light *lights = (light *)malloc(2 * sizeof(light));
    if (lights == NULL)
    {
        free(spheres);
        printf("Error allocate memory for lights");
        return 0;
    }
    lights[0] = light_create(vector_create(-20, 20, 20), 1.5);
    lights[1] = light_create(vector_create(30, 50, -25), 1.8);

    thread_pool_t pool = create_thread_pool(8);
    if (pool == NULL)
    {
        printf("Error creating thread pool");
        return 0;
    }

    render(pool, spheres, 4, lights, 2);

    free(spheres);
    free(lights);
    destroy_thread_pool(pool);

    return 0;
}