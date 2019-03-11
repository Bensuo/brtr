int global_index(const int offset_x, const int offset_y)
{
    int x = get_global_id(0) + offset_x;
    int y = get_global_id(1) + offset_y;
    x = x < 0 ? 0 : x >= get_global_size(0) ? get_global_size(0) - 1 : x;
    y = y < 0 ? 0 : y >= get_global_size(1) ? get_global_size(1) - 1 : y;
    return (x + y * get_global_size(0));
}
__kernel void combine(
    __global uchar4* direct,
    __global uchar4* indirect,
    __global uchar4* out
)

{
    const int idx = global_index(0,0);
    float4 d = convert_float3(direct[idx]) / 255.99f;
    float4 i = convert_float3(indirect[idx]) / 255.99f;
    float4 c = sqrt(d * i) * 255.99f;
    uchar4 o = convert_uchar4(c);
    o.w = 255;
    out[idx] = indirect[idx];
}