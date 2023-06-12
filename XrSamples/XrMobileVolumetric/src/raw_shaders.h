#ifndef RAW_SHADERS_H_
#define RAW_SHADERS_H_

namespace RawShaders {

    /// Basic sahders
const char basic_vertex[] = R"(#version 300 es
in  vec3 a_pos;
in  vec2 a_uv;
in  vec3 a_normal;

out vec2 v_uv;
out vec3 v_world_position;
out vec3 v_local_position;
out vec2 v_screen_position;

uniform mat4 u_vp_mat;
uniform mat4 u_model_mat;
uniform mat4 u_view_mat;
uniform mat4 u_proj_mat;

void main() {
    vec4 world_pos = u_model_mat * vec4(a_pos, 1.0);
    v_world_position = world_pos.xyz;
    v_local_position = a_pos;
    v_uv = a_uv;
    gl_Position = u_vp_mat * world_pos;
    v_screen_position = ((gl_Position.xy / gl_Position.w) + 1.0) / 2.0;
}
)";

const char quad_vertex[] = R"(#version 300 es
in  vec3 a_pos;
in  vec2 a_uv;
in  vec3 a_normal;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos.xy, 0.0, 1.0);
}
)";


const char volumetric_fragment[] = R"(#version 300 es
precision highp float;

in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

uniform vec3 u_camera_eye_local;
uniform highp sampler3D u_volume_map;
uniform highp sampler2D u_frame_color_attachment;

const int MAX_ITERATIONS = 100;
const float STEP_SIZE = 0.02;

vec4 render_volume() {
   vec3 ray_dir = -normalize(u_camera_eye_local - v_local_position);
   vec3 it_pos = vec3(0.0);
   vec4 final_color = vec4(0.0);
   float ray_step = 1.0 / float(MAX_ITERATIONS);

   // TODO: optimize iterations size, and step size
   for(int i = 0; i < MAX_ITERATIONS; i++) {
      if (final_color.a >= 0.95) {
         break;
      }
      vec3 sample_pos = ((u_camera_eye_local - it_pos) / 2.0) + 0.5;

      // Aboid clipping outside
      if (sample_pos.x < 0.0 || sample_pos.y < 0.0 || sample_pos.z < 0.0) {
         break;
      }
      if (sample_pos.x > 1.0 || sample_pos.y > 1.0 || sample_pos.z > 1.0) {
         break;
      }

      float depth = texture(u_volume_map, sample_pos).r;
      // Increase luminosity, only on the colors
      vec4 sample_color = vec4(04.6 * depth);
      sample_color.a = depth;

      final_color = final_color + (STEP_SIZE * (1.0 - final_color.a) * sample_color);
      it_pos = it_pos - (STEP_SIZE * ray_dir);
   }

   return vec4(final_color.xyz, 1.0);
}

void main() {
   //o_frag_color = v_local_position;
   o_frag_color = render_volume(); //*
   //o_frag_color = vec4(v_local_position / 2.0 + 0.5, 1.0);
   //o_frag_color = texture(u_frame_color_attachment, v_screen_position);
}
)";

const char volumetric_fragment_outside[] = R"(#version 300 es
precision highp float;
in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;
out vec4 o_frag_color;
uniform vec3 u_camera_eye_local;
uniform highp sampler3D u_volume_map;
const int MAX_ITERATIONS = 200;
const float STEP_SIZE = 0.005;
vec4 render_volume() {
   vec3 ray_dir = normalize(v_local_position - u_camera_eye_local);
   vec3 it_pos = v_local_position + ray_dir * 0.0001; // Start a bit further than the border
   vec4 final_color = vec4(0.0);

    int i = 0;
    for(; i < MAX_ITERATIONS; i++) {
        if (final_color.a >= 0.95) {
            break;
        }
        // Avoid going outside the texture
        if (it_pos.x < 0.0 || it_pos.y < 0.0 || it_pos.z < 0.0) {
            break;
        }
        if (it_pos.x > 1.0 || it_pos.y > 1.0 || it_pos.z > 1.0) {
            break;
        }
        float depth = texture(u_volume_map, it_pos).r;
        if (0.15 <= depth) {
            return vec4(it_pos, 1.0);
        }
        // Increase luminosity, only on the colors
        vec4 sample_color = vec4(04.6 * depth);
        sample_color.a = depth;
        final_color = final_color + (STEP_SIZE * (1.0 - final_color.a) * sample_color);
        it_pos = it_pos + (STEP_SIZE * ray_dir);
    }

    return vec4(vec3(0.0), 1.0);
    return vec4(it_pos / 2.0 + 0.5, 1.0);
    return vec4(final_color.xyz, 1.0);
}
void main() {
   //o_frag_color = v_local_position;
   o_frag_color = render_volume();
   //o_frag_color = vec4(normalize(u_camera_eye_local - v_local_position), 1.0);
   //o_frag_color = texture(u_frame_color_attachment, v_screen_position);
}
)";


const char isosurface_fragment_outside[] = R"(#version 300 es
precision highp float;
in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;
out vec4 o_frag_color;

uniform float u_time;
uniform vec3 u_camera_eye_local;
uniform highp sampler3D u_volume_map;
uniform highp sampler2D u_albedo_map; // Noise texture
uniform highp float u_density_threshold;

const int MAX_ITERATIONS = 350;
const float STEP_SIZE = 0.007; // 0.004 ideal for quality
const int NOISE_TEX_WIDTH = 100;

const float DELTA = 0.003;
const vec3 DELTA_X = vec3(DELTA, 0.0, 0.0);
const vec3 DELTA_Y = vec3(0.0, DELTA, 0.0);
const vec3 DELTA_Z = vec3(0.0, 0.0, DELTA);

vec3 gradient(in vec3 pos) {
    float x = texture(u_volume_map, pos + DELTA_X).r - texture(u_volume_map, pos - DELTA_X).r;
    float y = texture(u_volume_map, pos + DELTA_Y).r - texture(u_volume_map, pos - DELTA_Y).r;
    float z = texture(u_volume_map, pos + DELTA_Z).r - texture(u_volume_map, pos - DELTA_Z).r;

    return normalize(vec3(x, y, z) / vec3(DELTA * 2.0));
}

vec4 render_volume() {
    vec3 ray_dir = normalize(v_local_position - u_camera_eye_local);
    vec3 it_pos = v_local_position;
    // Add jitter
    vec3 jitter_addition = ray_dir * (texture(u_albedo_map, gl_FragCoord.xy / vec2(NOISE_TEX_WIDTH)).rgb * STEP_SIZE);
    it_pos = it_pos + jitter_addition;
    vec4 final_color = vec4(0.0);

    int i = 0;
    for(; i < MAX_ITERATIONS; i++) {
        if (final_color.a >= 0.95) {
            break;
        }
        // Avoid going outside the texture
        if (it_pos.x < 0.0 || it_pos.y < 0.0 || it_pos.z < 0.0) {
            break;
        }
        if (it_pos.x > 1.0 || it_pos.y > 1.0 || it_pos.z > 1.0) {
            break;
        }
        float depth = texture(u_volume_map, it_pos).r;
        if (u_density_threshold <= depth) {
            return vec4(gradient(it_pos - jitter_addition), 1.0);
      }

      it_pos = it_pos + (STEP_SIZE * ray_dir);
   }
   return vec4(vec3(0.0), 1.0);
}
void main() {
   //o_frag_color = texture(u_albedo_map, gl_FragCoord.xy / vec2(NOISE_TEX_WIDTH));
   o_frag_color = render_volume(); //*
   //o_frag_color = vec4(v_local_position / 2.0 + 0.5, 1.0);
   //o_frag_color = texture(u_frame_color_attachment, v_screen_position);
}
)";

    const char mar_shader[] = R"(#version 300 es
precision highp float;
in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

uniform float u_time;
uniform vec3 u_camera_eye_local;
uniform highp sampler3D u_volume_map;
uniform highp sampler2D u_albedo_map; // Noise texture
//uniform highp float u_density_threshold;

const int MAX_ITERATIONS = 200;
const int NOISE_TEX_WIDTH = 100;
const float STEP_SIZE = 0.80; // 0.004 ideal for quality
const float DELTA = 0.001;
const float SMALLEST_VOXEL = 0.0078125; // 2.0 / 256

const vec3 DELTA_X = vec3(DELTA, 0.0, 0.0);
const vec3 DELTA_Y = vec3(0.0, DELTA, 0.0);
const vec3 DELTA_Z = vec3(0.0, 0.0, DELTA);

vec3 gradient(in vec3 pos) {
    float x = texture(u_volume_map, pos + DELTA_X).r - texture(u_volume_map, pos - DELTA_X).r;
    float y = texture(u_volume_map, pos + DELTA_Y).r - texture(u_volume_map, pos - DELTA_Y).r;
    float z = texture(u_volume_map, pos + DELTA_Z).r - texture(u_volume_map, pos - DELTA_Z).r;
    return normalize(vec3(x, y, z) / vec3(DELTA * 2.0));
}

float get_int(in float f) {
    return float(int(f));
}

float get_level_of_size(in float size) {
    return 1.0 + log2(size);
}

const float mip_size_LUT[8] = float[8]( // POW(2.0, LEVEL-1)
    1.0, // 0
    1.0, // 1
    2.0, // 2
    4.0, // 3
    8.0, // 4
    16.0, // 5
    32.0,// 6
    64.0 // 7
);

const float mip_size_LUT_half[8] = float[8]( // 1.0 / POW(2.0, LEVEL-1)
    1.0, // 0
    1.0 / mip_size_LUT[1], // 1
    1.0 / mip_size_LUT[2], // 2
    1.0 / mip_size_LUT[3], // 3
    1.0 / mip_size_LUT[4], // 4
    1.0 / mip_size_LUT[5], // 5
    1.0 / mip_size_LUT[6],// 6
    1.0 / mip_size_LUT[7] // 7
);

const float mip_size_LUT_half_step[8] = float[8]( // 1.0 / POW(2.0, LEVEL-1)
    mip_size_LUT_half[0] * STEP_SIZE, // 0
    mip_size_LUT_half[1] * STEP_SIZE, // 1
    mip_size_LUT_half[2] * STEP_SIZE, // 2
    mip_size_LUT_half[3] * STEP_SIZE, // 3
    mip_size_LUT_half[4] * STEP_SIZE, // 4
    mip_size_LUT_half[5] * STEP_SIZE, // 5
    mip_size_LUT_half[6] * STEP_SIZE,// 6
    mip_size_LUT_half[7] * STEP_SIZE // 7
);

float get_size_of_miplevel(in float level) {
    //return get_int(pow(2.0, level - 1.0));
    return mip_size_LUT[int(level)];
}

void get_voxel_of_point_in_level(in vec3 point, in float mip_level, out vec3 origin, out vec3 size) {
    float voxel_proportions = mip_size_LUT_half[int(mip_level)];// The cube is sized 2,2,2 in world coords
    vec3 voxel_size = vec3(voxel_proportions);

    vec3 start_coords = point - mod(point, voxel_proportions);

    origin = start_coords;
    size = start_coords + voxel_size;
}

bool is_inside(in vec3 box_origin, in vec3 box_size, in vec3 position) {
    vec3 box_min = box_origin;
    vec3 box_max = box_origin + box_size;

    return all(greaterThan(position, box_min)) && all(lessThan(position, box_max));
}

bool is_inside_v2(in vec3 box_min, in vec3 box_max, in vec3 position) {
    return all(greaterThan(position, box_min)) && all(lessThan(position, box_max));
}


float get_distance(in float level) {
    return mip_size_LUT_half_step[int(level)];
    //return pow(2.0, level - 1.0) * SMALLEST_VOXEL * 0.025;
}

vec3 mrm() {
    // Raymarching conf
    vec3 ray_dir = normalize(v_local_position - u_camera_eye_local);
    vec3 pos = v_local_position - ray_dir * 0.001;
    vec3 jitter_addition = ray_dir * (texture(u_albedo_map, gl_FragCoord.xy / vec2(NOISE_TEX_WIDTH)).rgb * 0.03);
    pos += jitter_addition;

    // MRM
    float curr_mipmap_level = 5.0;
    float dist = 0.002; // Distance from start to sampling point
    float prev_dist = 0.0;
    vec3 prev_sample_pos = pos;
    vec3 sample_pos;

    //return vec3(dist_max);
    //float prev_mip_level = curr_mipmap_level;
    vec3 prev_voxel_min = vec3(0.0);
    vec3 prev_voxel_max = vec3(1.0);

    vec3 box_min = vec3(0.0), box_max = vec3(1.0);

    int i = 0;
    for(; i < MAX_ITERATIONS; i++) {
        vec3 sample_pos = pos + (dist * ray_dir);

        // Early out
        if (!is_inside_v2(box_min, box_max, sample_pos)) {
            break;
        }


        float depth = textureLod(u_volume_map, sample_pos, curr_mipmap_level).r;
        if (depth > 0.15) { // There is a block
            if (curr_mipmap_level == 0.0) {
                return sample_pos;
                //break;
                //return gradient(sample_pos) * 0.5 + 0.5;
            }
            get_voxel_of_point_in_level(sample_pos, curr_mipmap_level, prev_voxel_min, prev_voxel_max);
            curr_mipmap_level = curr_mipmap_level - 1.0;
            // go back  one step
            dist = prev_dist;
        } else { // Ray is unblocked
            dist += get_distance(curr_mipmap_level);
        }
        prev_dist = dist;
    }

    //return vec3(float(i)  / float(MAX_ITERATIONS));
    return vec3(0.0);
}

void main() {
   o_frag_color = vec4(mrm(), 1.0);
})";


    const char mar_sweep_shader[] = R"(#version 300 es
precision highp float;
in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

uniform float u_time;
uniform vec3 u_camera_eye_local;
uniform highp sampler3D u_volume_map;
uniform highp sampler2D u_albedo_map; // Noise texture
//uniform highp float u_density_threshold;

const int MAX_ITERATIONS = 80;
const int NOISE_TEX_WIDTH = 100;
const float STEP_SIZE = 0.80; // 0.004 ideal for quality
const float DELTA = 0.001;
const float SMALLEST_VOXEL = 0.0078125; // 2.0 / 256
const float RAY_STEP_SIZE = 0.007;

const vec3 DELTA_X = vec3(DELTA, 0.0, 0.0);
const vec3 DELTA_Y = vec3(0.0, DELTA, 0.0);
const vec3 DELTA_Z = vec3(0.0, 0.0, DELTA);

vec3 gradient(in vec3 pos) {
    float x = texture(u_volume_map, pos + DELTA_X).r - texture(u_volume_map, pos - DELTA_X).r;
    float y = texture(u_volume_map, pos + DELTA_Y).r - texture(u_volume_map, pos - DELTA_Y).r;
    float z = texture(u_volume_map, pos + DELTA_Z).r - texture(u_volume_map, pos - DELTA_Z).r;
    return normalize(vec3(x, y, z) / vec3(DELTA * 2.0));
}

float get_int(in float f) {
    return float(int(f));
}

float get_level_of_size(in float size) {
    return 1.0 + log2(size);
}

const float mip_size_LUT[8] = float[8]( // POW(2.0, LEVEL-1)
    1.0, // 0
    1.0, // 1
    2.0, // 2
    4.0, // 3
    8.0, // 4
    16.0, // 5
    32.0,// 6
    64.0 // 7
);

const float mip_size_LUT_half[8] = float[8]( // 1.0 / POW(2.0, LEVEL-1)
    1.0, // 0
    1.0 / mip_size_LUT[1], // 1
    1.0 / mip_size_LUT[2], // 2
    1.0 / mip_size_LUT[3], // 3
    1.0 / mip_size_LUT[4], // 4
    1.0 / mip_size_LUT[5], // 5
    1.0 / mip_size_LUT[6],// 6
    1.0 / mip_size_LUT[7] // 7
);

const float mip_size_LUT_half_step[8] = float[8]( // 1.0 / POW(2.0, LEVEL-1)
    mip_size_LUT_half[0] * STEP_SIZE, // 0
    mip_size_LUT_half[1] * STEP_SIZE, // 1
    mip_size_LUT_half[2] * STEP_SIZE, // 2
    mip_size_LUT_half[3] * STEP_SIZE, // 3
    mip_size_LUT_half[4] * STEP_SIZE, // 4
    mip_size_LUT_half[5] * STEP_SIZE, // 5
    mip_size_LUT_half[6] * STEP_SIZE,// 6
    mip_size_LUT_half[7] * STEP_SIZE // 7
);

float get_size_of_miplevel(in float level) {
    //return get_int(pow(2.0, level - 1.0));
    return mip_size_LUT[int(level)];
}

void get_voxel_of_point_in_level(in vec3 point, in float mip_level, out vec3 origin, out vec3 size) {
    float voxel_proportions = mip_size_LUT_half[int(mip_level)];// The cube is sized 2,2,2 in world coords
    vec3 voxel_size = vec3(voxel_proportions);

    vec3 start_coords = point - mod(point, voxel_proportions);

    origin = start_coords;
    size = start_coords + voxel_size;
}

bool is_inside(in vec3 box_origin, in vec3 box_size, in vec3 position) {
    vec3 box_min = box_origin;
    vec3 box_max = box_origin + box_size;

    return all(greaterThan(position, box_min)) && all(lessThan(position, box_max));
}

bool is_inside_v2(in vec3 box_min, in vec3 box_max, in vec3 position) {
    return all(greaterThan(position, box_min)) && all(lessThan(position, box_max));
}


float get_distance(in float level) {
    return mip_size_LUT_half_step[int(level)];
    //return pow(2.0, level - 1.0) * SMALLEST_VOXEL * 0.025;
}

vec3 mrm() {
    // Raymarching conf
    vec3 ray_dir = normalize(v_local_position - u_camera_eye_local);
    vec3 pos = v_local_position - ray_dir * 0.001;
    vec3 jitter_addition = ray_dir * (texture(u_albedo_map, gl_FragCoord.xy / vec2(NOISE_TEX_WIDTH)).rgb * 0.02);
    pos += jitter_addition;

    // MRM
    float curr_mipmap_level = 6.0;
    float dist = 0.002; // Distance from start to sampling point
    float prev_dist = 0.0;
    vec3 prev_sample_pos = pos;
    vec3 sample_pos;

    //return vec3(dist_max);
    //float prev_mip_level = curr_mipmap_level;
    vec3 prev_voxel_min = vec3(0.0);
    vec3 prev_voxel_max = vec3(1.0);

    vec3 box_min = vec3(0.0), box_max = vec3(1.0);

    // MAR
    int i = 0;
    for(; i < MAX_ITERATIONS || curr_mipmap_level > 1.0; i++) {
        vec3 sample_pos = pos + (dist * ray_dir);

        // Early out
        if (!is_inside_v2(box_min, box_max, sample_pos)) {
            return vec3(0.0);
        }


        float depth = textureLod(u_volume_map, sample_pos, curr_mipmap_level).r;
        if (depth > 0.15) { // There is a block
            curr_mipmap_level = curr_mipmap_level - 1.0;
            // go back  one step
            dist = ((dist - prev_dist) / 2.0) + prev_dist;
        } else { // Ray is unblocked
            dist += get_distance(curr_mipmap_level);
        }
        prev_dist = dist;
    }

    //dist -= jitter_addition;
    // RAYMARCHING
    for(; i < 150; i++) {
        vec3 sample_pos = pos + (dist * ray_dir);

        // Early out
        if (!is_inside_v2(box_min, box_max, sample_pos)) {
            return vec3(0.0);
        }

        float depth = texture(u_volume_map, sample_pos).r;
        if (depth >= 0.15) {
            return sample_pos;
        }
        dist += RAY_STEP_SIZE;
   }

    //return vec3(float(i)  / float(MAX_ITERATIONS));
    return vec3(0.0);//pos + (dist * ray_dir);
}

void main() {
   o_frag_color = vec4(mrm(), 1.0);
})";

const char local_fragment[] = R"(#version 300 es
precision highp float;

in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

void main() {
    o_frag_color = vec4(v_local_position, 1.0);
}
)";


const char basic_fragment[] = R"(#version 300 es
precision highp float;

in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

void main() {
    o_frag_color = vec4(1.0, 1.0, 0.0, 1.0);
}
)";

};

#endif // RAW_SHADERS_H_
