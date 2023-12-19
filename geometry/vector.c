#include "vector.h"
#include "math.h"
vec2 vector2_create(float x, float y)
{
    vec2 vec;
    vec.x = x;
    vec.y = y;
    return vec;
}

vec3 vector_create(float x, float y, float z)
{
    vec3 vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
}

vec3 vector_addition(vec3 lhs, vec3 rhs)
{
    lhs.x = lhs.x + rhs.x;
    lhs.y = lhs.y + rhs.y;
    lhs.z = lhs.z + rhs.z;
    return lhs;
}

vec3 vector_diff(vec3 lhs, vec3 rhs)
{
    lhs.x = lhs.x - rhs.x;
    lhs.y = lhs.y - rhs.y;
    lhs.z = lhs.z - rhs.z;
    return lhs;
}

float vector_scalar_product(vec3 lhs, vec3 rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

vec3 vector_multiplication(vec3 lhs, float rhs)
{
    lhs.x = lhs.x * rhs;
    lhs.y = lhs.y * rhs;
    lhs.z = lhs.z * rhs;
    return lhs;
}

float vector_norm(vec3 vec)
{
    return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

vec3 vector_normalize(vec3 vec)
{
    vec = vector_multiplication(vec, 1 / vector_norm(vec));
    return vec;
}