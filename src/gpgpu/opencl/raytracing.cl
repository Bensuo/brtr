#define DEPTH 5
#define RANDOM_VALUES (1 << 17)
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
    int parent;
    int children[2];
    struct AABB aabb;
    unsigned leaf_node[2];
};

struct LeafNode
{
    int parent;
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

struct DirectionalLight
{
    float3 direction;
    float3 colour;
};

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
        return true;
    }

    return false;
}

float max_component(float3 val)
{
    return max(val.x, max(val.y, val.z));
}
float min_component(float3 val)
{
    return min(val.x, min(val.y, val.z));
}
bool intersects_aabb(Ray r, struct AABB aabb, float* t)
{
    float3 t0 = (aabb.min - r.origin) * r.inv_dir;
    float3 t1 = (aabb.max - r.origin) * r.inv_dir;

    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);

    float min_val = max_component(tmin);
    float max_val = min_component(tmax);

    bool check = false;
    if (min_val < 0)
        check = max_val <= *t;
    else
        check = min_val <= *t;

    return max_val >= max(min_val, 0.0f);
}
struct RayResult trace(Ray ray, __global struct Node* nodes, int num_nodes, __global struct LeafNode* leaf_nodes, bool early_exit)
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
        barrier(CLK_GLOBAL_MEM_FENCE);
        iterations++;

        bool hit = intersects_aabb(ray, cached_node.aabb, &t);
        if (hit)
        {
            for (int i = 0; i < 2; i++)
            {
                if (cached_node.children[i] == -1)
                {
                    global struct LeafNode* leaf = leaf_nodes + cached_node.leaf_node[i];
                    struct LeafNode cached_leaf = *leaf;

                    hit = intersects_triangle(ray, cached_leaf.tri, &result, &t);
                    if (hit)
                    {
                        result.mat = cached_leaf.material;
                        result.hit = hit;
                        if (early_exit)
                            return result;
                    }
                }
                else
                {
                    global struct Node* n = nodes + cached_node.children[i];

                    *stack_ptr++ = n;
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
    int index = x + y * get_global_size(0) + offset;
    int total_size = get_global_size(0) * get_global_size(1);
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

void create_basis(float3* normal, float3* nt, float3* nb)
{
    if (fabs(normal->x) > fabs(normal->y))
        *nt = (float3)(normal->z, 0, -normal->x) /
              sqrt(normal->x * normal->x + normal->z * normal->z);
    else
        *nt = (float3)(0, -normal->z, normal->y) /
              sqrt(normal->y * normal->y + normal->z * normal->z);

    *nb = cross(*normal, *nt);
}
__kernel void ray_trace(
    __global struct Node* nodes,
    int num_nodes,
    __global struct LeafNode* leaf_nodes,
    __global struct Material* materials,
    __global uchar4* out_direct,
    __constant float3* camera,
    __global float* random_seeds,
    struct DirectionalLight dir_light)
{
    float aspect_ratio = (float)get_global_size(0) / (float)get_global_size(1);
    float scale = 1.0f;
    float u = (((float)get_global_id(0) / get_global_size(0)));
    float v = (float)get_global_id(1) / get_global_size(1);

    Ray r;
    r.origin = camera[0];
    r.direction = normalize(camera[1] + u * camera[2] + v * camera[3] - camera[0]);
    r.inv_dir = half_recip(r.direction);

    struct RayResult result;
    int hit_mats[DEPTH];
    float dir_light_hit[DEPTH];
    float reflect_dot[DEPTH];
    for (int i = 0; i < DEPTH; i++)
    {
        hit_mats[i] = -1;
        dir_light_hit[i] = 0.0f;
        reflect_dot[i] = 0.0f;
    }
    for (int i = 0; i < DEPTH; i++)
    {
        barrier(NULL);
        result = trace(r, nodes, num_nodes, leaf_nodes, false);

        if (result.hit)
        {
            hit_mats[i] = result.mat;

            // Directional Light
            Ray light_ray;
            light_ray.origin = result.hit_point;
            light_ray.direction = -dir_light.direction;
            light_ray.inv_dir = half_recip(light_ray.direction);
            struct RayResult light_result =
                trace(light_ray, nodes, num_nodes, leaf_nodes, true);
            if (!light_result.hit)
            {
                dir_light_hit[i] = fmax(
                    0.0f, dot(normalize(-dir_light.direction), result.hit_normal));
            }
            if (result.reflected)
            {
                r.origin = result.hit_point;
                r.direction = normalize(
                    r.direction - 2.0f * dot(r.direction, result.hit_normal) *
                                      result.hit_normal);
                float3 nb, nt;
                create_basis(&result.hit_normal, &nt, &nb);
                float3 sample = (get_rand_float3(random_seeds, wrapping_index(i)));
                float3 sample_normal = (float3)(
                    sample.x * nb.x + sample.y * result.hit_normal.x + sample.z * nt.x,
                    sample.x * nb.y + sample.y * result.hit_normal.y + sample.z * nt.y,
                    sample.x * nb.z + sample.y * result.hit_normal.z + sample.z * nt.z);
                r.direction = normalize(
                    sample_normal * materials[result.mat].roughness +
                    r.direction * (1 - materials[result.mat].roughness));
                reflect_dot[i] = max(0.0f, dot(result.hit_normal, r.direction));

                r.inv_dir = half_recip(r.direction);
            }
        }
        else
        {
            break;
        }
    }
    float3 indirect = (float3)(0.0f);
    float3 direct = (float3)(0.0f);
    float pdf = (float)(1 / (2 * M_PI));
    for (int i = DEPTH - 1; i >= 0; i--)
    {
        if (hit_mats[i] >= 0)
        {
            struct Material mat = materials[hit_mats[i]];

            indirect = reflect_dot[i] * direct / pdf;
            direct = mat.emissive + (dir_light.colour * dir_light_hit[i]);
            direct = (direct / (float)M_PI + 2 * indirect) * mat.diffuse;
        }
        else
        {
            direct = (float3)(1.0f);
        }
    }

    direct = sqrt(direct);

    out_direct[global_index(0, 0)] = (uchar4)(convert_uchar3(direct * 255.99f), 255);
}