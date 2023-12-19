#ifndef VECTOR_H
#define VECTOR_H

typedef struct vec2
{
    float x;
    float y;
} vec2;

typedef struct vec3
{
    float x;
    float y;
    float z;
} vec3;

#ifdef __cplusplus
extern "C"
{
#endif
    vec2 vector2_create(float x, float y);
    vec3 vector_create(float x, float y, float z);

    vec3 vector_addition(vec3 lhs, vec3 rhs);

    vec3 vector_diff(vec3 lhs, vec3 rhs);

    float vector_scalar_product(vec3 lhs, vec3 rhs);

    vec3 vector_multiplication(vec3 lhs, float rhs);

    float vector_norm(vec3 vec);

    vec3 vector_normalize(vec3 vec);

#ifdef __cplusplus
}
#endif // extern "C"
#endif