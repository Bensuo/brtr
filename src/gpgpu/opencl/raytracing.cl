#define DEPTH 5
#define RANDOM_VALUES (1 << 16)
typedef enum TagObjectType
{
    sphere,
    triangle,
    rectangle
} ObjectType;
typedef struct TagSphere
{
    float radius;
    float3 pos;
    float3 emi;
    float3 colour;
} Sphere;

typedef struct TagRay
{
    float3 origin;
    float3 direction;
    float3 inv_dir;
} Ray;

struct Vertex
{
    float3 position;
    float3 normal;
};
struct Triangle
{
    struct Vertex verts[3];
};
struct AABB
{
    float3 min;
    float3 max;
};
struct Material
{
    float3 diffuse;
    float roughness;
    float3 emissive;
};

struct Node
{
    int children[4];
    struct AABB aabb;
    unsigned leaf_node;
};

struct LeafNode
{
    struct AABB aabb;
    struct Triangle tri;
    unsigned material;
    unsigned morton;
};
struct RayResult
{
    bool reflected;
    float3 hit_point;
    float3 hit_normal;
    float3 new_dir;
    bool hit;
    unsigned mat;
    uchar3 colour;
};

struct Camera
{
    float3 position;
    float3 lower_left;
    float3 horizontal;
    float3 vertical;
};

struct Light
{
    float3 position;
    float3 colour;
    float radius;
};
bool intersects_sphere(Ray ray, Sphere sphere, float* t)
{
    float3 rayToCenter = sphere.pos - ray.origin;
    // float a = dot(ray.direction, ray.direction);
    float b = dot(rayToCenter, ray.direction);
    float c = dot(rayToCenter, rayToCenter) - sphere.radius * sphere.radius;
    float disc = b * b - c;

    if (disc < 0.0f)
        return false;
    else
        *t = b - sqrt(disc);
    if (*t < 0.0f)
    {
        *t = b + sqrt(disc);
        if (*t < 0.0f)
            return false;
    }
    else
        return true;
}
bool intersects_triangle(Ray r, struct Triangle tri, struct RayResult* result, float* t)
{
    
    float3 vertex0 = tri.verts[0].position;
    float3 vertex1 = tri.verts[1].position;
    float3 vertex2 = tri.verts[2].position;
    float3 edge1, edge2, h, s, q, norm;
    float a, f, u, v;
    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    norm = normalize(cross(edge1, edge2));
    h = cross(r.direction, edge2);
    a = dot(h, edge1);
    if (fabs(a) < FLT_EPSILON)
        return false;
    f = 1.0f / a;
    s = r.origin - vertex0;
    u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    q = cross(s, edge1);
    v = f * dot(r.direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    float t2 = f * dot(edge2, q);

    float d = dot(r.direction, norm);
    if (t2 > 0.001f && d < 0.0f && t2 < *t)
    {
        
        (*t) = t2;
        result->reflected = true;
        result->hit_point = r.origin + r.direction * t2;
        result->hit_normal = norm;
        // result->colour = convert_uchar3(0.5f * (float3)(norm.x + 1, norm.y + 1, norm.z + 1) * 255.0f);
        return true;
    }

    return false;
    /* float t = 0.0f;
    float3 v0v1 = tri.verts[1].position - tri.verts[0].position;
    float3 v0v2 = tri.verts[2].position - tri.verts[0].position;
    float3 norm = cross(v0v1, v0v2);

    float norm_dot = dot(norm, r.direction);
    if(fabs(norm_dot) < FLT_EPSILON) return false; // they're parallel

    float d = dot(norm, tri.verts[0].position);

    t = (dot(norm, r.origin) + d) / norm_dot;
    if(t < 0) return false; // triangle is behind

    float3 P = r.origin + t*r.direction;

    float3 C;

    float3 edge0 = tri.verts[1].position - tri.verts[0].position;
    float3 vp0 = P - tri.verts[0].position;
    C = cross(edge0, vp0);
    if(dot(norm, C) < 0) return false; // P is on right side

    float3 edge1 = tri.verts[2].position - tri.verts[1].position;
    float3 vp1 = P - tri.verts[1].position;
    C = cross(edge1, vp1);
    if(dot(norm, C) < 0) return false; // P is on right side

    float3 edge2 = tri.verts[0].position - tri.verts[2].position;
    float3 vp2 = P - tri.verts[2].position;
    C = cross(edge2, vp2);
    if(dot(norm, C) < 0) return false; // P is on right side

    result->colour = convert_uchar3(0.5f*(float3)(norm.x + 1, norm.y + 1, norm.z
    +1)*255.0f); return true; */
}
bool intersects_aabb(Ray r, struct AABB aabb, float* t)
{
    float tx1 = (aabb.min.x - r.origin.x) / r.inv_dir.x;
    float tx2 = (aabb.max.x - r.origin.x) / r.inv_dir.x;

    float tmin = min(tx1, tx2);
    float tmax = max(tx1, tx2);

    float ty1 = (aabb.min.y - r.origin.y) / r.inv_dir.y;
    float ty2 = (aabb.max.y - r.origin.y) / r.inv_dir.y;

    tmin = max(tmin, min(ty1, ty2));
    tmax = min(tmax, max(ty1, ty2));

    float tz1 = (aabb.min.z - r.origin.z) / r.inv_dir.z;
    float tz2 = (aabb.max.z - r.origin.z) / r.inv_dir.z;

    tmin = max(tmin, min(tz1, tz2));
    tmax = min(tmax, max(tz1, tz2));

    /* if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {
            printf("test: %f,%f,%f,%f,%f,%f\n", tx1, tx2, ty1, ty2, tz1, tz2, tmin, tmax);
        } */
    if (tmax >= max(0.0f, tmin))
    {
        return true;
        /* result->hit = true;
        result->colour = (uchar3)(255,0,0); */
    }

    return false;
}
struct RayResult trace(Ray ray, __global struct Node* nodes, int num_nodes, __global struct LeafNode* leaf_nodes)
{
    struct RayResult result;
    result.hit = false;
    float t = FLT_MAX;

    global struct Node* stack[64];
    global struct Node** stack_ptr = stack;

    *stack_ptr++ = NULL;

    int iterations = 0;
    global struct Node* node = nodes;
    do
    {
        struct Node cached_node = *node;
        iterations++;
        if (cached_node.leaf_node > 0)
        {
            
            global struct LeafNode* leaf = leaf_nodes + cached_node.leaf_node - 1;
            struct LeafNode cached_leaf = *leaf;
            bool hit = intersects_triangle(ray, cached_leaf.tri, &result, &t);
            if (hit)
            {
                result.mat = cached_leaf.material;
                result.hit = hit;
            }
        }
        else
        {
            bool hit = intersects_aabb(ray, cached_node.aabb, &t);
            if (hit)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (cached_node.children[i] != 0)
                    {
                        global struct Node* n = nodes + cached_node.children[i];
                        hit = intersects_aabb(ray, n->aabb, &t);
                        if (hit)
                        {
                            *stack_ptr++ = n;
                        }
                    }
                }
            }
        }
        node = *--stack_ptr; // pop next node

    } while (node != NULL);

    return result;
}
int global_index(const int offset_x, const int offset_y)
{
    int x = get_global_id(0) + offset_x;
    int y = get_global_id(1) + offset_y;
    x = x < 0 ? 0 : x >= get_global_size(0) ? get_global_size(0) - 1 : x;
    y = y < 0 ? 0 : y >= get_global_size(1) ? get_global_size(1) - 1 : y;
    return (x + y * get_global_size(0));
}

int wrapping_index(int offset)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int index  = x + y * get_global_size(0)  + offset;
    int total_size = get_global_size(0) * get_global_size(1);
    //return index > total_size ? index - total_size : index;
    return index % RANDOM_VALUES;
}
float3 colour(Ray r)
{
    float t = 0.5f + (r.direction.y);
    float3 start_colour = (float3)(1.0f);
    float3 end_colour = (float3){0.5f, 0.95f, 1.0f};
    float3 out_colour = (1.0f - t) * start_colour + t * end_colour;
    return out_colour;
}
float3 get_rand_float3(__global float* seeds, int index)
{
    return vload3(index, seeds);
}
/* float squared_length(float3 f)
{
    return f.x*f.x + f.y*f.y + f.z*f.z;
}
float3 random_in_unit_sphere(uint2* seed) {
    float3 p;
    do {

        p = 2.0f*rand(seed) - (float3)(1,1,1);
    } while (squared_length(p) >= 1.0);
    return p;
} */

__kernel void ray_trace(
    __global struct Node* nodes,
    int num_nodes,
    __global struct LeafNode* leaf_nodes,
    __global struct Material* materials,
    __global uchar4* out_direct,
    __constant float3* camera,
    __global float* random_seeds,
    __global struct Light* lights,
        __global uchar4* out_indirect)
{
    float aspect_ratio = (float)get_global_size(0) / (float)get_global_size(1);
    float scale = 1.0f;
    float u = (((float)get_global_id(0) / get_global_size(0)));
    float v = (float)get_global_id(1) / get_global_size(1);
    

    Ray r;
    r.origin = camera[0];
    r.direction = normalize(camera[1] + u * camera[2] + v * camera[3] - camera[0]);
    r.inv_dir = r.direction;
    // r.inv_dir = (float3){1.0f / r.direction.x, 1.0f / r.direction.y, 1.0f / r.direction.z};
    struct RayResult result;
    int hit_mats[DEPTH];
    
    for (int i = 0; i < DEPTH; i++)
    {
        hit_mats[i] = -1;
    }
    for (int i = 0; i < DEPTH; i++)
    {
        
        barrier(NULL);
        result = trace(r, nodes, num_nodes, leaf_nodes);

        if (result.hit)
        {
            hit_mats[i] = result.mat;
/*             struct Material mat = materials[result.mat];
            if(i == 0)
            {
                accumulated_colour0 = mat.emissive;
                accumulated_colour1 = mat.diffuse;
                if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {
            printf("emissive: %.2f, %.2f, %.2f\n", mat.emissive.x, mat.emissive.y, mat.emissive.z);
            printf("diffuse: %.2f, %.2f, %.2f\n", mat.diffuse.x, mat.diffuse.y, mat.diffuse.z);
            printf("colour0: %.2f, %.2f, %.2f\n", accumulated_colour0.x, accumulated_colour0.y, accumulated_colour0.z);
            printf("colour1: %.2f, %.2f, %.2f\n", accumulated_colour1.x, accumulated_colour1.y, accumulated_colour1.z);
        }
            }
            else
            {
                
                accumulated_colour1 *= (mat.emissive + mat.diffuse);
                if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {
            printf("emissive: %.2f, %.2f, %.2f\n", mat.emissive.x, mat.emissive.y, mat.emissive.z);
            printf("diffuse: %.2f, %.2f, %.2f\n", mat.diffuse.x, mat.diffuse.y, mat.diffuse.z);
            printf("colour0: %.2f, %.2f, %.2f\n", accumulated_colour0.x, accumulated_colour0.y, accumulated_colour0.z);
            printf("colour1: %.2f, %.2f, %.2f\n", accumulated_colour1.x, accumulated_colour1.y, accumulated_colour1.z);
        }
            }
            // accumulated_colour *= 0.9f;*/
            // Directional Light
            
            if (result.reflected)
            {
                //int seed = random_seeds[global_index(0,0)];
                
                r.origin = result.hit_point;
                //r.direction = normalize(-2 * dot(r.direction, result.hit_normal) * result.hit_normal + r.direction);
                r.direction = normalize(r.direction - 2.0f * dot(r.direction, result.hit_normal) * result.hit_normal);
               /*  if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {

            printf("origin: %.2f, %.2f, %.2f\n", r.origin.x, r.origin.y, r.origin.z);
            printf("reflect dir: %.2f, %.2f, %.2f\n", r.direction.x, r.direction.y, r.direction.z);
            printf("normal: %.2f, %.2f, %.2f\n", result.hit_normal.x, result.hit_normal.y, result.hit_normal.z);
        } */
                r.direction = normalize(r.origin + r.direction + (get_rand_float3(random_seeds, wrapping_index(i))+ get_rand_float3(random_seeds, wrapping_index(i + 3))+ get_rand_float3(random_seeds, wrapping_index(i + 6)) / 3)* materials[result.mat].roughness - r.origin);
                
                //random_seeds[global_index(0,0)] = seed;
                
                r.inv_dir = r.direction;

                /* if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {
            printf("new dir: %.2f, %.2f, %.2f\n", r.direction.x, r.direction.y, r.direction.z);
        } */

            } 
        }
        else
        {
            break;
        }
    }
    float3 indirect = (float3)(1.0f);
    float3 direct = (float3)(1.0f);
    for(int i = DEPTH - 1; i >= 0; i--)
    {
        
        if(hit_mats[i] >= 0)
        {
            struct Material mat = materials[hit_mats[i]];
            if(i > DEPTH)
            {
        indirect = mat.emissive + mat.diffuse * indirect;
            }
    else{

        indirect = mat.emissive + mat.diffuse * indirect;

     }
        }
        else{
            if(i > DEPTH)
            indirect = colour(r) * 0.7f;
            else{
                indirect = colour(r) * 0.7f;
            }
        }
        
    }
    if(get_global_id(0) == get_global_size(0)/2 && get_global_id(1) == get_global_size(1)/2)
        {
            //printf("emissive: %.2f, %.2f, %.2f\n", mat.emissive.x, mat.emissive.y, mat.emissive.z);
            //printf("diffuse: %.2f, %.2f, %.2f\n", mat.diffuse.x, mat.diffuse.y, mat.diffuse.z);
            //printf("out: %.2f, %.2f, %.2f\n", out_colour.x, out_colour.y, out_colour.z);
        }
    //
    /* struct AABB test;
    test.position = (float3)(0);
    test.min = (float3)(-2.5f);
    test.max = (float3)(2.5f);
    test.material = 0;
    intersects_aabb(r, test, &result); */
    /* if(!result.hit)
    {
        output = colour(r);
    }
    else{
        //output = (uchar3)(255,0,0);
        output = result.colour;
        //output = convert_uchar3(materials[result.mat].diffuse * 255.0f);
    } */
    direct = sqrt(direct);
    indirect = sqrt(indirect);
    out_direct[global_index(0, 0)] = (uchar4)(convert_uchar3(direct * 255.99f), 255);
    out_indirect[global_index(0, 0)] = (uchar4)(convert_uchar3(indirect * 255.99f), 255);
    /* uchar3 output;
    Sphere sphere1;
    sphere1.radius = 0.4f;
    sphere1.pos = (float3)(0.0f, 0.0f, 0.0f);
    sphere1.colour = (float3)(0.9f, 0.3f, 0.0f);
    float t = 1e20;
    intersects_sphere(r, sphere1, &t);
    

    

    if(t > 1e19)
    {
        output = colour(r);
    }
    else
    {
        float3 normal = normalize((r.origin + r.direction * t) - sphere1.pos);
        output = convert_uchar3(0.5f*(float3)(normal.x + 1, normal.y + 1, normal.z +1)*255.0f);
        //output = convert_uchar3(sphere1.colour * 255.0f);
    }

    out[global_index(0,0)] = (uchar4)(output, 255); */
}