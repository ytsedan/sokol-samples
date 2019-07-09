//------------------------------------------------------------------------------
//  sgl-sapp.c
//  Rendering via sokol_gl.h
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#define SOKOL_GL_IMPL
#include "sokol_gl.h"
#include "imgui.h"
#include "imgui_font.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

#define SAMPLE_COUNT (4)

static bool show_test_window = true;
static bool show_another_window = false;
static const sg_pass_action pass_action = {
    .colors[0] = {
        .action = SG_ACTION_CLEAR,
        .val = { 0.0f, 0.0f, 0.0f, 1.0f }
    }
};
static sg_image img;
/* a sokol_gl pipeline object for 3d rendering */
static sgl_pipeline pip_3d;

static void init(void) {
    sg_desc desc = {
        .gl_force_gles2 = sapp_gles2(),
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view,
    };
    sg_setup(&desc);

    // setup sokol-imgui, but provide our own font
    simgui_desc_t simgui_desc = { };
    simgui_desc.no_default_font = true;
    simgui_desc.dpi_scale = sapp_dpi_scale();
    simgui_desc.sample_count = SAMPLE_COUNT;
    simgui_setup(&simgui_desc);

    // configure Dear ImGui with our own embedded font
    auto& io = ImGui::GetIO();
    ImFontConfig fontCfg;
    fontCfg.FontDataOwnedByAtlas = false;
    fontCfg.OversampleH = 2;
    fontCfg.OversampleV = 2;
    fontCfg.RasterizerMultiply = 1.5f;
    io.Fonts->AddFontFromMemoryTTF(dump_font, sizeof(dump_font), 16.0f, &fontCfg);

    // create font texture for the custom font
    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    sg_image_desc img_desc = { };
    img_desc.width = font_width;
    img_desc.height = font_height;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    img_desc.min_filter = SG_FILTER_LINEAR;
    img_desc.mag_filter = SG_FILTER_LINEAR;
    img_desc.content.subimage[0][0].ptr = font_pixels;
    img_desc.content.subimage[0][0].size = font_width * font_height * 4;
    io.Fonts->TexID = (ImTextureID)(uintptr_t) sg_make_image(&img_desc).id;

    /* setup sokol-gl */
    sgl_desc_t sgl_desc = { .sample_count = SAMPLE_COUNT };
    sgl_setup(&sgl_desc);

    /* a checkerboard texture */
    uint32_t pixels[8][8];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            pixels[y][x] = ((y ^ x) & 1) ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    img_desc = (sg_image_desc){
        .width = 8,
        .height = 8,
        .content.subimage[0][0] = {
            .ptr = pixels,
            .size = sizeof(pixels)
        }
    };
    img = sg_make_image(&img_desc);

    /* create a pipeline object for 3d rendering, with less-equal
       depth-test and cull-face enabled, not that we don't provide
       a shader, vertex-layout, pixel formats and sample count here,
       these are all filled in by sokol-gl
    */
    sg_pipeline_desc pip_desc = {
        .depth_stencil = {
            .depth_write_enabled = true,
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
        },
        .rasterizer = {
            .cull_mode = SG_CULLMODE_BACK,
            .sample_count = SAMPLE_COUNT
        }
    };
    pip_3d = sgl_make_pipeline(&pip_desc);
}

static void draw_triangle(void) {
    sgl_defaults();
    sgl_begin_triangles();
    sgl_v2f_c3b( 0.0f,  0.5f, 255, 0, 0);
    sgl_v2f_c3b(-0.5f, -0.5f, 0, 0, 255);
    sgl_v2f_c3b( 0.5f, -0.5f, 0, 255, 0);
    sgl_end();
}

static void draw_quad(void) {
    static float angle_deg = 0.0f;
    float scale = 1.0f + sinf(sgl_rad(angle_deg)) * 0.5f;
    angle_deg += 1.0f;
    sgl_defaults();
    sgl_rotate(sgl_rad(angle_deg), 0.0f, 0.0f, 1.0f);
    sgl_scale(scale, scale, 1.0f);
    sgl_begin_quads();
    sgl_v2f_c3b( -0.5f, -0.5f,  255, 255, 0);
    sgl_v2f_c3b(  0.5f, -0.5f,  0, 255, 0);
    sgl_v2f_c3b(  0.5f,  0.5f,  0, 0, 255);
    sgl_v2f_c3b( -0.5f,  0.5f,  255, 0, 0);
    sgl_end();
}

/* vertex specification for a cube with colored sides and texture coords */
static void cube(void) {
    sgl_begin_quads();
    sgl_c3f(1.0f, 0.0f, 0.0f);
        sgl_v3f_t2f(-1.0f,  1.0f, -1.0f, -1.0f,  1.0f);
        sgl_v3f_t2f( 1.0f,  1.0f, -1.0f,  1.0f,  1.0f);
        sgl_v3f_t2f( 1.0f, -1.0f, -1.0f,  1.0f, -1.0f);
        sgl_v3f_t2f(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f);
    sgl_c3f(0.0f, 1.0f, 0.0f);
        sgl_v3f_t2f(-1.0, -1.0,  1.0, -1.0f,  1.0f);
        sgl_v3f_t2f( 1.0, -1.0,  1.0,  1.0f,  1.0f);
        sgl_v3f_t2f( 1.0,  1.0,  1.0,  1.0f, -1.0f);
        sgl_v3f_t2f(-1.0,  1.0,  1.0, -1.0f, -1.0f);
    sgl_c3f(0.0f, 0.0f, 1.0f);
        sgl_v3f_t2f(-1.0, -1.0,  1.0, -1.0f,  1.0f);
        sgl_v3f_t2f(-1.0,  1.0,  1.0,  1.0f,  1.0f);
        sgl_v3f_t2f(-1.0,  1.0, -1.0,  1.0f, -1.0f);
        sgl_v3f_t2f(-1.0, -1.0, -1.0, -1.0f, -1.0f);
    sgl_c3f(1.0f, 0.5f, 0.0f);
        sgl_v3f_t2f(1.0, -1.0,  1.0, -1.0f,   1.0f);
        sgl_v3f_t2f(1.0, -1.0, -1.0,  1.0f,   1.0f);
        sgl_v3f_t2f(1.0,  1.0, -1.0,  1.0f,  -1.0f);
        sgl_v3f_t2f(1.0,  1.0,  1.0, -1.0f,  -1.0f);
    sgl_c3f(0.0f, 0.5f, 1.0f);
        sgl_v3f_t2f( 1.0, -1.0, -1.0, -1.0f,  1.0f);
        sgl_v3f_t2f( 1.0, -1.0,  1.0,  1.0f,  1.0f);
        sgl_v3f_t2f(-1.0, -1.0,  1.0,  1.0f, -1.0f);
        sgl_v3f_t2f(-1.0, -1.0, -1.0, -1.0f, -1.0f);
    sgl_c3f(1.0f, 0.0f, 0.5f);
        sgl_v3f_t2f(-1.0,  1.0, -1.0, -1.0f,  1.0f);
        sgl_v3f_t2f(-1.0,  1.0,  1.0,  1.0f,  1.0f);
        sgl_v3f_t2f( 1.0,  1.0,  1.0,  1.0f, -1.0f);
        sgl_v3f_t2f( 1.0,  1.0, -1.0, -1.0f, -1.0f);
    sgl_end();
}

static void draw_cubes(void) {
    static float rot[2] = { 0.0f, 0.0f };
    rot[0] += 1.0f;
    rot[1] += 2.0f;

    sgl_defaults();
    sgl_load_pipeline(pip_3d);

    sgl_matrix_mode_projection();
    sgl_perspective(sgl_rad(45.0f), 1.0f, 0.1f, 100.0f);

    sgl_matrix_mode_modelview();
    sgl_translate(0.0f, 0.0f, -12.0f);
    sgl_rotate(sgl_rad(rot[0]), 1.0f, 0.0f, 0.0f);
    sgl_rotate(sgl_rad(rot[1]), 0.0f, 1.0f, 0.0f);
    cube();
    sgl_push_matrix();
        sgl_translate(0.0f, 0.0f, 3.0f);
        sgl_scale(0.5f, 0.5f, 0.5f);
        sgl_rotate(-2.0f * sgl_rad(rot[0]), 1.0f, 0.0f, 0.0f);
        sgl_rotate(-2.0f * sgl_rad(rot[1]), 0.0f, 1.0f, 0.0f);
        cube();
        sgl_push_matrix();
            sgl_translate(0.0f, 0.0f, 3.0f);
            sgl_scale(0.5f, 0.5f, 0.5f);
            sgl_rotate(-3.0f * sgl_rad(2*rot[0]), 1.0f, 0.0f, 0.0f);
            sgl_rotate(3.0f * sgl_rad(2*rot[1]), 0.0f, 0.0f, 1.0f);
            cube();
        sgl_pop_matrix();
    sgl_pop_matrix();
}

static void draw_tex_cube(void) {
    static float frame_count = 0.0f;
    frame_count += 1.0f;
    float a = sgl_rad(frame_count);

    // texture matrix rotation and scale
    float tex_rot = 0.5f * a;
    const float tex_scale = 1.0f + sinf(a) * 0.5f;

    // compute an orbiting eye-position for testing sgl_lookat()
    float eye_x = sinf(a) * 6.0f;
    float eye_z = cosf(a) * 6.0f;
    float eye_y = sinf(a) * 3.0f;

    sgl_defaults();
    sgl_load_pipeline(pip_3d);

    sgl_enable_texture();
    sgl_texture(img);

    sgl_matrix_mode_projection();
    sgl_perspective(sgl_rad(45.0f), 1.0f, 0.1f, 100.0f);
    sgl_matrix_mode_modelview();
    sgl_lookat(eye_x, eye_y, eye_z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    sgl_matrix_mode_texture();
    sgl_rotate(tex_rot, 0.0f, 0.0f, 1.0f);
    sgl_scale(tex_scale, tex_scale, 1.0f);
    cube();
}

static void frame(void) {
    /* compute viewport rectangles so that the views are horizontally
       centered and keep a 1:1 aspect ratio
    */
    const int dw = sapp_width();
    const int dh = sapp_height();
    const int ww = dh/2; /* not a bug */
    const int hh = dh/2;
    const int x0 = dw/2 - hh;
    const int x1 = dw/2;
    const int y0 = 0;
    const int y1 = dh/2;
    /* all sokol-gl functions except sgl_draw() can be called anywhere in the frame */
    sgl_viewport(x0, y0, ww, hh, true);
    draw_triangle();
//    sgl_viewport(x1, y0, ww, hh, true);
//    draw_quad();
//    sgl_viewport(x0, y1, ww, hh, true);
//    draw_cubes();
//    sgl_viewport(x1, y1, ww, hh, true);
//    draw_tex_cube();
//    sgl_viewport(0, 0, dw, dh, true);

    const int width = sapp_width();
    const int height = sapp_height();
    simgui_new_frame(width, height, 1.0/60.0);

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    static float f = 0.0f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    if (ImGui::Button("Test Window")) show_test_window ^= 1;
    if (ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("NOTE: programmatic quit isn't supported on web and mobile");
    if (ImGui::Button("Soft Quit")) {
        sapp_request_quit();
    }
    if (ImGui::Button("Hard Quit")) {
        sapp_quit();
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window) {
        ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window) {
        ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow();
    }

    // 4. Prepare and conditionally open the "Really Quit?" popup
    if (ImGui::BeginPopupModal("Really Quit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Do you really want to quit?\n");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            sapp_quit();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    /* Render the sokol-gfx default pass, all sokol-gl commands
       that happened so far are rendered inside sgl_draw(), and this
       is the only sokol-gl function that must be called inside
       a sokol-gfx begin/end pass pair.
       sgl_draw() also 'rewinds' sokol-gl for the next frame.
    */
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sgl_draw();
    simgui_render();
    sg_end_pass();
    sg_commit();
}

void input(const sapp_event* event) {
    simgui_handle_event(event);
}

static void cleanup(void) {
    sgl_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = input,
        .width = 1024,
        .height = 768,
        .sample_count = SAMPLE_COUNT,
        .high_dpi = true,
        .gl_force_gles2 = true,
        .window_title = "sokol_gl.h (sokol-app)",
    };
}
