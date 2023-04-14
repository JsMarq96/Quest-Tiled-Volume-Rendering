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
    float curr_mipmap_level = 7.0;
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

const char world_fragment_shader[] = R"(#version 300 es
precision highp float;

in vec2 v_uv;
in vec3 v_world_position;
in vec3 v_local_position;
in vec2 v_screen_position;

out vec4 o_frag_color;

void main() {
    o_frag_color = vec4(v_world_position, 1.0);
}
)";

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




// ===========================
// COMPUTE SHADERS
// ===========================
const char surface_find_compute_shader[] = R"(#version 300 es
layout(std430, binding = 1) buffer vertices_surfaces {
    vec4 vertices[];
};
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform highp sampler3D u_volume_map;

const vec3 DELTAS[8] = vec3[8](
    vec3(0,0,0),
    vec3(1,0,0), //
    vec3(0,1,0),
    vec3(1,1,0),
    vec3(0,0,1),
    vec3(1,0,1),
    vec3(0,1,1),
    vec3(1,1,1)
);

ivec3 num_work_groups = ivec3(gl_NumWorkGroups.xyz);

int num_work_groups_z_p2 = (num_work_groups.z * num_work_groups.z);

int get_index_of_position(in ivec3 position) {
    return int(position.x + position.y * num_work_groups.y + position.z * num_work_groups_z_p2);
}

void main() {
	ivec3 curr_index = ivec3(gl_GlobalInvocationID.xyz);
    vec3 works_size =  vec3(gl_NumWorkGroups.xyz);
    vec3 world_position = vec3(curr_index) / works_size;
    uint index = get_index_of_position(curr_index);

    vec3 point = vec3(0.0);
    int axis_seed = 0;
    int axis_count = 0;
    for(int i = 0; i < 8; i++) {
        vec3 delta_pos = (world_position + (DELTAS[i])/works_size);
        float density = texture(u_volume_map, delta_pos).r;
        if (density > 0.15) {
            point += delta_pos;
            axis_seed |= 1 << i;
            axis_count++;
        }
    }
    // TODO: better point fiding!
    vec3 value = vec3(0.0);
    if (axis_count > 0 && axis_count < 8) {
        value = ((point / axis_count));
    } else {
        axis_seed = 0;
    }
    vertices[index] = vec4(value, float(axis_seed));
})";


const char surface_triangularize_compute_shader[] = R"(#version 300 es

struct sSurfacePoint {
    int is_surface;
    vec3 position;
};

layout(std430, binding = 1) buffer vertices_surfaces {
    vec4 vertices[];
};

layout(std430, binding = 2) buffer raw_mesh {
    vec4 mesh_vertices[];
};

layout(std430, binding = 3) buffer vertex_count {
    uint mesh_vertices_count;
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

const ivec3 TEST_AXIS_Z[4] = ivec3[4](
    ivec3(1, 0, 0),
    ivec3(0, 1, 0),
    ivec3(-1, 0, 0),
    ivec3(0, -1, 0)
);

const ivec3 TEST_AXIS_X[4] = ivec3[4](
    ivec3(0, 1, 0),
    ivec3(0, 0, 1),
    ivec3(0, -1, 0),
    ivec3(0, 0, -1)
);

const ivec3 TEST_AXIS_Y[4] = ivec3[4](
    ivec3(1, 0, 0),
    ivec3(0, 0, 1),
    ivec3(-1, 0, 0),
    ivec3(0, 0, -1)
);

ivec3 num_work_groups = ivec3(gl_NumWorkGroups.xyz);

int num_work_groups_z_p2 = (num_work_groups.z * num_work_groups.z);

int get_index_of_position(in ivec3 position) {
    return int(position.x + position.y * num_work_groups.y + position.z * num_work_groups_z_p2);
}

void test_triangle_x(int index, in ivec3 curr_pos, in vec4 current_point) {
    int t1_v1 = get_index_of_position(curr_pos + TEST_AXIS_X[0]), t1_v2 = get_index_of_position(curr_pos + TEST_AXIS_X[1]);
    int t2_v1 = get_index_of_position(curr_pos + TEST_AXIS_X[2]), t2_v2 = get_index_of_position(curr_pos + TEST_AXIS_X[3]);

    // Test first triangle
    if (vertices[t1_v1].w != 0.0 && vertices[t1_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t1_v1];
        mesh_vertices[start_vert+2] = vertices[t1_v2];
    }

    // Test second triangle
    if (vertices[t2_v1].w != 0.0 && vertices[t2_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t2_v1];
        mesh_vertices[start_vert+2] = vertices[t2_v2];
    }
}

void test_triangle_y(int index, in ivec3 curr_pos, in vec4 current_point) {
    int t1_v1 = get_index_of_position(curr_pos + TEST_AXIS_Y[0]), t1_v2 = get_index_of_position(curr_pos + TEST_AXIS_Y[1]);
    int t2_v1 = get_index_of_position(curr_pos + TEST_AXIS_Y[2]), t2_v2 = get_index_of_position(curr_pos + TEST_AXIS_Y[3]);

   // Test first triangle
    if (vertices[t1_v1].w != 0.0 && vertices[t1_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t1_v1];
        mesh_vertices[start_vert+2] = vertices[t1_v2];
    }

    // Test second triangle
    if (vertices[t2_v1].w != 0.0 && vertices[t2_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t2_v1];
        mesh_vertices[start_vert+2] = vertices[t2_v2];
    }
}

void test_triangle_z(int index, in ivec3 curr_pos, in vec4 current_point) {
    int t1_v1 = get_index_of_position(curr_pos + TEST_AXIS_Z[0]), t1_v2 = get_index_of_position(curr_pos + TEST_AXIS_Z[1]);
    int t2_v1 = get_index_of_position(curr_pos + TEST_AXIS_Z[2]), t2_v2 = get_index_of_position(curr_pos + TEST_AXIS_Z[3]);

    // Test first triangle
    if (vertices[t1_v1].w != 0.0 && vertices[t1_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t1_v1];
        mesh_vertices[start_vert+2] = vertices[t1_v2];
    }

    // Test second triangle
    if (vertices[t2_v1].w != 0.0 && vertices[t2_v2].w != 0.0) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = current_point;
        mesh_vertices[start_vert+1] = vertices[t2_v1];
        mesh_vertices[start_vert+2] = vertices[t2_v2];
    }
}

/*
void test_triangle_y(in ivec3 curr_pos, in sSurfacePoint current_point) {
    int t1_v1 = get_index_of_position(curr_pos + TEST_AXIS_Y[0]), t1_v2 = get_index_of_position(curr_pos + TEST_AXIS_Y[1]);
    // Test first triangle
    if (vertices[t1_v1].is_surface != 0 && vertices[t1_v1].is_surface != 255 && vertices[t1_v2].is_surface != 0 && vertices[t1_v2].is_surface != 255) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = vec4(current_point.position, 0.0);
        mesh_vertices[start_vert+1] = vec4(vertices[t1_v1].position, 0.0);
        mesh_vertices[start_vert+2] = vec4(vertices[t1_v2].position, 0.0);
    }

    int t2_v1 = get_index_of_position(curr_pos + TEST_AXIS_Y[2]), t2_v2 = get_index_of_position(curr_pos + TEST_AXIS_Y[3]);
    // Test first triangle
    if (vertices[t2_v1].is_surface != 0 && vertices[t2_v1].is_surface != 255 && vertices[t2_v2].is_surface != 0 && vertices[t2_v2].is_surface != 255) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = vec4(current_point.position, 0.0);
        mesh_vertices[start_vert+1] = vec4(vertices[t2_v1].position, 0.0);
        mesh_vertices[start_vert+2] = vec4(vertices[t2_v2].position, 0.0);
    }
}

void test_triangle_z(in ivec3 curr_pos, in sSurfacePoint current_point) {
    int t1_v1 = get_index_of_position(curr_pos + TEST_AXIS_Z[0]), t1_v2 = get_index_of_position(curr_pos + TEST_AXIS_Z[1]);
    // Test first triangle
    if (vertices[t1_v1].is_surface != 0 && vertices[t1_v1].is_surface != 255 && vertices[t1_v2].is_surface != 0 && vertices[t1_v2].is_surface != 255) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = vec4(current_point.position, 0.0);
        mesh_vertices[start_vert+1] = vec4(vertices[t1_v1].position, 0.0);
        mesh_vertices[start_vert+2] = vec4(vertices[t1_v2].position, 0.0);
    }

    int t2_v1 = get_index_of_position(curr_pos + TEST_AXIS_Z[2]), t2_v2 = get_index_of_position(curr_pos + TEST_AXIS_Z[3]);
    // Test first triangle
    if (vertices[t2_v1].is_surface != 0 && vertices[t2_v1].is_surface != 255 && vertices[t2_v2].is_surface != 0 && vertices[t2_v2].is_surface != 255) { // Early out
        uint start_vert = atomicAdd(mesh_vertices_count, 3);
        mesh_vertices[start_vert] = vec4(current_point.position, 0.0);
        mesh_vertices[start_vert+1] = vec4(vertices[t2_v1].position, 0.0);
        mesh_vertices[start_vert+2] = vec4(vertices[t2_v2].position, 0.0);
    }
}*/

void main() {
    ivec3 post = ivec3(gl_GlobalInvocationID);
    int index = get_index_of_position(post);

    vec4 current_point = vertices[index];

    //atomicAdd(mesh_vertices_count, 1);

    if (current_point.w == 0.0) { // Early out
        return;
    }

    test_triangle_x(index, post, current_point);
    test_triangle_y(index, post, current_point);
    test_triangle_z(index, post, current_point);
})";
};

#endif // RAW_SHADERS_H_
