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
    return xx * 4 + yy * 2 + zz;
}

int global_index(const int offset_x, const int offset_y)
{
    int x = get_global_id(0) + offset_x;
    int y = get_global_id(1) + offset_y;
    x = x < 0 ? 0 : x >= get_global_size(0) ? get_global_size(0) - 1 : x;
    y = y < 0 ? 0 : y >= get_global_size(1) ? get_global_size(1) - 1 : y;
    return (x + y * get_global_size(0));
}

unsigned long concatenate(unsigned x, unsigned y)
{
    return (unsigned long)x << 32 | (unsigned long)y;
}

int delta(int i, int j, __global unsigned* morton_codes, int num_objects)
{
    if (i < 0 || i >= num_objects || j < 0 || j >= num_objects) {
        return 0;
    }

    ulong mI = ((ulong)morton_codes[i]) << 32 | (ulong)i;
    ulong mJ = ((ulong)morton_codes[j]) << 32 | (ulong)j;
    if(mI == mJ) clz((ulong)i ^ (ulong)j);
    return clz(mI ^ mJ);
}
int2 calc_range(__global unsigned* morton_codes, int num_objects, int idx)
{
    if (idx == 0)
    {
        return (int2){0, num_objects - 1};
    }

    int prev_0 = delta(idx, idx-1, morton_codes, num_objects);
    int next_0 = delta(idx, idx+1, morton_codes, num_objects);

    int direction = sign((float)next_0 - (float)prev_0);

    int d_min = delta(idx, idx - direction, morton_codes, num_objects);

    int lmax = 2;

    while (delta(idx, idx + lmax * direction, morton_codes, num_objects) > d_min)
    {
        lmax = lmax * 2;
    }

    // lmax = direction>0 ? num_objects-1 - idx : idx;
    int l = 0;

    int t = lmax;
    while(t >= 1)
    {
        int d = delta(idx, idx + (l+t) * direction, morton_codes, num_objects);
        if(d > d_min)
        {
            l += t;
        }
        t = t/2;
    }
    int jdx = idx + l * direction;

    
    int2 range = (int2)(0,0);
    range.x = min(idx, jdx);
    range.y = max(idx, jdx);
    return range;
}

int calc_split(__global unsigned* morton_codes, int first, int last)
{
    ulong firstCode = (ulong)morton_codes[first] << 32 | (ulong)first;
    ulong lastCode = (ulong)morton_codes[last] << 32 | (ulong)last;

    if (firstCode == lastCode)
    {
        return (first + last) >> 1;
    }

    int commonPrefix = clz(firstCode ^ lastCode);

    int split = first;
    int step = last - first;

    do
    {
        step = (step + 1) >> 1;
        int newSplit = split + step;

        if (newSplit < last)
        {
            ulong splitCode = (ulong)morton_codes[newSplit] << 32 | (ulong)newSplit;
            int splitPrefix = clz(firstCode ^ splitCode);
            if (splitPrefix > commonPrefix)
            {
                split = newSplit;
            }
        }
    } while (step > 1);

    return split;
}
__kernel void generate_hierarchy(
    __global struct Node* internal_nodes,
    __global struct LeafNode* leaf_nodes,
    __global unsigned* morton_codes)
{
    int idx = get_global_id(0);

    int2 range = calc_range(morton_codes, get_global_size(0) + 1, idx);
    int first = range.x;
    int last = range.y;

    int split = calc_split(morton_codes, first, last);
    
    int childA, childB;

    if (split == first)
    {
        internal_nodes[idx].children[0] = -1;
        internal_nodes[idx].leaf_node[0] = split;
        leaf_nodes[split].parent = idx;
    }
    else
    {
        internal_nodes[idx].children[0] = split;
        internal_nodes[split].parent = idx;
    }
    if (split + 1 == last)
    {
        internal_nodes[idx].children[1] = -1;
        internal_nodes[idx].leaf_node[1] = split + 1;
        leaf_nodes[split + 1].parent = idx;
    }
    else
    {
        internal_nodes[idx].children[1] = split + 1;
        internal_nodes[split + 1].parent = idx;
    }

    if (idx == 0)
    {
        internal_nodes[idx].parent = -1;
    }
}

__kernel void expand_bounding_boxes(
    __global struct Node* internal_nodes,
    __global struct LeafNode* leaf_nodes,
    __global int* node_flags)
{
    int idx = get_global_id(0);
    if (idx < get_global_size(0) - 1)
        node_flags[idx] = 0;
    struct AABB combined, childA, childB;
    int next = leaf_nodes[idx].parent;
    childA = leaf_nodes[idx].aabb;
    printf("leaf bounds: %v3f  %v3f\n", childA.min, childA.max);
    while (1)
    {
        
        if(atomic_inc(node_flags + next) != 0) return;
        printf("next: %d\n", next);
        struct Node node = internal_nodes[next];
        if (node.children[0] == -1)
        {
            childA = leaf_nodes[node.leaf_node[0]].aabb;
            if(idx == 0)
        {
            printf("a_leaf: %d %v3f\n", node.leaf_node[0], leaf_nodes[node.leaf_node[0]].aabb.min);
        }
        }
        else
        {
            childA = internal_nodes[node.children[0]].aabb;
            if(idx == 0)
        {
            printf("a_int: %d %v3f\n", node.children[0], internal_nodes[node.children[0]].aabb.min);
        }
        }

        if (node.children[1] == -1)
        {
            childB = leaf_nodes[node.leaf_node[1]].aabb;
            if(idx == 0)
        {
            printf("b_leaf: %d %v3f\n", node.leaf_node[1], leaf_nodes[node.leaf_node[1]].aabb.min);
        }
        }
        else
        {
            childB = internal_nodes[node.children[1]].aabb;
            if(idx == 0)
        {
            printf("b_int: %d %v3f\n", node.children[1], internal_nodes[node.children[1]].aabb.min);
        }
        }
        node.aabb.min = fmin(childA.min, childB.min);
        node.aabb.max = fmax(childA.max, childB.max);
        
        internal_nodes[next] = node;
        barrier(CLK_GLOBAL_MEM_FENCE);
        if (node.parent == -1)
            break;
            next = node.parent;
            
    }
}
__kernel void calc_bounding_boxes(__global struct LeafNode* leaf_nodes)
{
    struct LeafNode node = leaf_nodes[get_global_id(0)];
    float3 min = (float3)(FLT_MAX);
    float3 max = (float3)(FLT_MIN);
    float3 pos;
    for (int i = 0; i < 3; i++)
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
    pos = min + (max - min) / 2;
    node.morton = calc_morton(pos.x, pos.y, pos.z);
    leaf_nodes[get_global_id(0)] = node;
}