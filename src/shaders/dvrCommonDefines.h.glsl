#ifndef RAY_STEP_SIZE
    #define RAY_STEP_SIZE   0.00475 //0.004 // 0.0033
#endif

#define LEVOY_ISO_SURFACE       0
#define F2B_COMPOSITE           1
#define XRAY                    2
#define MRI                     3

#define HOUNSFIELD_UNIT_SCALE ( 65535.0 / 4095.0 )

#define BRICK_BLOCK_DIM     12.0 // better than 8 and 16 ...
#define RECIP_BRICK_BLOCK_DIM (1.0 / BRICK_BLOCK_DIM)

#ifdef __cplusplus
using vec4 = std::array<float, 4>;
struct
#else
layout (std140) uniform 
#endif
LightParameters {
    vec4 lightDir;
    vec4 lightColor;
    float ambient;
    float diffuse;
    float specular;
    float dummy;
};
