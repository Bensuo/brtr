#define BLUR_RADIUS 2
int local_index(const int offset_x, const int offset_y)
{
    int x = get_local_id(0) + (BLUR_RADIUS -1) + offset_x;
    int y = get_local_id(1) + (BLUR_RADIUS - 1) + offset_y;
    return  x + y * (get_local_size(0) + (BLUR_RADIUS-1)*2);
}

int global_index(const int offset_x, const int offset_y)
{
    int x = get_global_id(0) + offset_x;
    int y = get_global_id(1) + offset_y;
    x = x < 0 ? 0 : x >= get_global_size(0) ? get_global_size(0)-1 : x;
    y = y < 0 ? 0 : y >= get_global_size(1) ? get_global_size(1)-1 : y;
    return (x + y * get_global_size(0));
}

int filter_index(const int x, const int y)
{
    return x + y * (BLUR_RADIUS *2-1);
}
uchar3 average_pixel(
    __local uchar4* cache, 
    __global uchar* out,
    __constant float* filter
    )
{
    uchar3 cached[9];
    uchar* values[9];
    uchar3 result = (uchar3)(0,0,0);
    for(int j = 0; j < BLUR_RADIUS * 2-1; ++j)
    {
        for(int i = 0; i < BLUR_RADIUS * 2 -1; ++i)
        {
            cached[i + (BLUR_RADIUS * 2 -1) * j] = cache[local_index(i - BLUR_RADIUS + 1, j - BLUR_RADIUS + 1)].xyz;
            values[i + (BLUR_RADIUS * 2 -1) * j] = ((uchar*)&cached[i + (BLUR_RADIUS * 2 -1) * j]);
        }
    }

    for(int i = 0; i < 3; i++)
    {
        uchar uiMin = min(values[0][i], values[1][i]);
        uchar uiMax = max(values[0][i], values[1][i]);
        values[0][i] = uiMin;
        values[1][i] = uiMax;

        uiMin = min(values[3][i], values[2][i]);
        uiMax = max(values[3][i], values[2][i]);
        values[3][i] = uiMin;
        values[2][i] = uiMax;

        uiMin = min(values[2][i], values[0][i]);
        uiMax = max(values[2][i], values[0][i]);
        values[2][i] = uiMin;
        values[0][i] = uiMax;

        uiMin = min(values[3][i], values[1][i]);
        uiMax = max(values[3][i], values[1][i]);
        values[3][i] = uiMin;
        values[1][i] = uiMax;

        uiMin = min(values[1][i], values[0][i]);
        uiMax = max(values[1][i], values[0][i]);
        values[1][i] = uiMin;
        values[0][i] = uiMax;

        uiMin = min(values[3][i], values[2][i]);
        uiMax = max(values[3][i], values[2][i]);
        values[3][i] = uiMin;
        values[2][i] = uiMax;

        uiMin = min(values[5][i], values[4][i]);
        uiMax = max(values[5][i], values[4][i]);
        values[5][i] = uiMin;
        values[4][i] = uiMax;

        uiMin = min(values[7][i], values[8][i]);
        uiMax = max(values[7][i], values[8][i]);
        values[7][i] = uiMin;
        values[8][i] = uiMax;

        uiMin = min(values[6][i], values[8][i]);
        uiMax = max(values[6][i], values[8][i]);
        values[6][i] = uiMin;
        values[8][i] = uiMax;

        uiMin = min(values[6][i], values[7][i]);
        uiMax = max(values[6][i], values[7][i]);
        values[6][i] = uiMin;
        values[7][i] = uiMax;

        uiMin = min(values[4][i], values[8][i]);
        uiMax = max(values[4][i], values[8][i]);
        values[4][i] = uiMin;
        values[8][i] = uiMax;

        uiMin = min(values[4][i], values[6][i]);
        uiMax = max(values[4][i], values[6][i]);
        values[4][i] = uiMin;
        values[6][i] = uiMax;

        uiMin = min(values[5][i], values[7][i]);
        uiMax = max(values[5][i], values[7][i]);
        values[5][i] = uiMin;
        values[7][i] = uiMax;

        uiMin = min(values[4][i], values[5][i]);
        uiMax = max(values[4][i], values[5][i]);
        values[4][i] = uiMin;
        values[5][i] = uiMax;

        uiMin = min(values[6][i], values[7][i]);
        uiMax = max(values[6][i], values[7][i]);
        values[6][i] = uiMin;
        values[7][i] = uiMax;

        uiMin = min(values[0][i], values[8][i]);
        uiMax = max(values[0][i], values[8][i]);
        values[0][i] = uiMin;
        values[8][i] = uiMax;

        values[4][i] = max(values[0][i], values[4][i]);
        values[5][i] = max(values[1][i], values[5][i]);

        values[6][i] = max(values[2][i], values[6][i]);
        values[7][i] = max(values[3][i], values[7][i]);

        values[4][i] = min(values[4][i], values[6][i]);
        values[5][i] = min(values[5][i], values[7][i]);

        ((uchar*)&result)[i] |= min(values[4][i], values[5][i]);
    }
    return result;
}

void cache_pixels(__global uchar* in, __local uchar4* cache, const int x, const int y, const int lx, const int ly)
{
    cache[local_index(0, 0)] = vload4(global_index(0,0), in);
    if(lx < (BLUR_RADIUS-1))
    {
        cache[local_index(-(BLUR_RADIUS-1), 0)] = vload4(global_index(-(BLUR_RADIUS-1), 0), in);
        if(ly < BLUR_RADIUS-1)
        {
            cache[local_index(-(BLUR_RADIUS-1), -(BLUR_RADIUS-1))] = vload4(global_index(-(BLUR_RADIUS-1), -(BLUR_RADIUS-1)), in);
        }
        if(ly >= (get_local_size(1) - (BLUR_RADIUS-1)))
        {
            cache[local_index(-(BLUR_RADIUS-1), (BLUR_RADIUS-1))] = vload4(global_index(-(BLUR_RADIUS-1), (BLUR_RADIUS-1)), in);
        }
    }
    if(ly < (BLUR_RADIUS - 1))
    {
        cache[local_index(0, -(BLUR_RADIUS-1))] = vload4(global_index(0, -(BLUR_RADIUS-1)), in);
    }
    if(lx >= (get_local_size(0) - (BLUR_RADIUS-1)))
    {
        cache[local_index(BLUR_RADIUS-1, 0)] = vload4(global_index((BLUR_RADIUS-1), 0), in);
        if(ly < BLUR_RADIUS-1)
        {
            cache[local_index((BLUR_RADIUS-1), -(BLUR_RADIUS-1))] = vload4(global_index((BLUR_RADIUS-1), -(BLUR_RADIUS-1)), in);
        }
        if(ly >= (get_local_size(1) - (BLUR_RADIUS-1)))
        {
            cache[local_index((BLUR_RADIUS-1), (BLUR_RADIUS-1))] = vload4(global_index((BLUR_RADIUS-1), (BLUR_RADIUS-1)), in);
        }
    }
    if(ly >= (get_local_size(1) - (BLUR_RADIUS-1)))
    {
        cache[local_index(0, BLUR_RADIUS-1)] = vload4(global_index(0,(BLUR_RADIUS-1)), in);
    }
    
}
__kernel void denoise(
    __global uchar* in, 
    __global uchar4* out,
     __constant float* filter,
     __local uchar4* cache)
{
    
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int lx = get_local_id(0);
    const int ly = get_local_id(1);
    
    cache_pixels(in, cache, x, y, lx, ly);
    barrier(CLK_LOCAL_MEM_FENCE);

    uchar3 new_val = average_pixel(cache, out, filter);
    
    out[global_index(0,0)] = (uchar4)(new_val, 255);
    //out[global_index(0,0)] = vload4(global_index(0,0), in);

}