#include "sokol_app.h"
#include "sokol_gfx.h"
#include "dbgui/dbgui.h"
#include "quad-sapp.glsl.h"
#include <stdlib.h>
#include <stdio.h>

static sg_pass_action pass_action = {
    .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.0f, 0.0f, 0.0f, 1.0f } }
};
static sg_pipeline pip;
static sg_bindings bind;
static bool do_jiggle;
static bool update_in_frame;

void init(void) {
    sg_setup(&(sg_desc){
        .gl_force_gles2 = sapp_gles2(),
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    __dbgui_setup(1);

    /* a vertex buffer */
    float vertices[] = {
        // positions            colors
        -0.5f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 0.0f, 1.0f,
    };
    bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .label = "quad-vertices",
        .usage = SG_USAGE_DYNAMIC
    });
    sg_update_buffer(bind.vertex_buffers[0], vertices, sizeof(vertices));

    /* an index buffer with 2 triangles */
    uint16_t indices[] = { 0, 1, 2,  0, 2, 3 };
    bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .content = indices,
        .label = "quad-indices"
    });

    /* a shader (use separate shader sources here */
    sg_shader shd = sg_make_shader(quad_shader_desc());

    /* a pipeline state object */
    pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_color0].format   = SG_VERTEXFORMAT_FLOAT4
            }
        },
        .label = "quad-pipeline"
    });
}

double rand_between(double min, double max) {
    return ((double)rand()/RAND_MAX) * (max - min) + min;
}

void jiggle() {
    float vertices[] = {
        // positions            colors
        -0.5f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 0.0f, 1.0f,
    };
    for (int i = 0; i < 28; i += 7) {
        vertices[i] += (float) rand_between(-0.1, 0.1);
        vertices[i + 1] += (float) rand_between(-0.1, 0.1);
    }
    sg_update_buffer(bind.vertex_buffers[0], vertices, sizeof(vertices));
}

void frame(void) {
    if (do_jiggle)
    {
        do_jiggle = false;
        printf("jigging in frame callback\n");
        jiggle();
    }

    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(pip);
    sg_apply_bindings(&bind);
    sg_draw(0, 6, 1);
    __dbgui_draw();
    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* evt, void* ud) {
    __dbgui_event(evt);
    if (evt->type == SAPP_EVENTTYPE_KEY_UP) {
        if (evt->key_code == SAPP_KEYCODE_J) {
            if (update_in_frame) {
                do_jiggle = true;
            } else {
                printf("jigging in event callback\n");
                jiggle();
            }
        }
        if (evt->key_code == SAPP_KEYCODE_T) {
            update_in_frame = !update_in_frame;
            printf("doing jiggle in frame callback: %d\n", update_in_frame);
        }
    }
}

void cleanup(void) {
    __dbgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_userdata_cb = event,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 600,
        .gl_force_gles2 = true,
        .window_title = "Quad (sokol-app)",
    };
}
