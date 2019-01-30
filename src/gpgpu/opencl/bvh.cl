struct Vertex
{
    float3 position;
    float3 normal;
};
struct Triangle
{
    struct Vertex verts[3];
};
struct AABB{
    float3 min;
    float3 max;
};

struct Node{
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
unsigned expand_bits(unsigned v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

unsigned calc_morton(float x, float y, float z)
{
    x = min(max(x * 1024.0f, 0.0f), 1023.0f);
    y = min(max(y * 1024.0f, 0.0f), 1023.0f);
    z = min(max(z * 1024.0f, 0.0f), 1023.0f);
    unsigned xx = expand_bits((unsigned)x);
    unsigned yy = expand_bits((unsigned)y);
    unsigned zz = expand_bits((unsigned)z);
    return xx *4 + yy*2 + zz;
}

int global_index(const int offset_x, const int offset_y)
{
    int x = get_global_id(0) + offset_x;
    int y = get_global_id(1) + offset_y;
    x = x < 0 ? 0 : x >= get_global_size(0) ? get_global_size(0)-1 : x;
    y = y < 0 ? 0 : y >= get_global_size(1) ? get_global_size(1)-1 : y;
    return (x + y * get_global_size(0));
}
__kernel void calc_bounding_boxes(
    __global struct LeafNode* leaf_nodes
)
{
    struct LeafNode node = leaf_nodes[get_global_id(0)]; 
    float3 min = (float3)(FLT_MAX);
    float3 max = (float3)(FLT_MIN);
    float3 pos;
    for(int i = 0; i < 3; i++)
    {
        float3 pos = node.tri.verts[i].position;

        min.x = pos.x < min.x ? pos.x : min.x;
        min.y = pos.y < min.y ? pos.y : min.y;
        min.z = pos.z < min.z ? pos.z : min.z;
        max.x = pos.x > max.x ? pos.x : max.x;
        max.y = pos.y > max.y ? pos.y : max.y;
        max.z = pos.z > max.z ? pos.z : max.z;

    }
    node.aabb.min = min;
    node.aabb.max = max;
    pos = min + (max-min) / 2;
    node.morton = calc_morton(pos.x, pos.y, pos.z);
    leaf_nodes[get_global_id(0)] = node;
}