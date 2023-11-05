/*
 * Copyright (C) 2023  Renan S. Silva, aka h3nnn4n
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cglm/call.h>
#include <cglm/cglm.h>

#include <stb_image.h>

#include <pcg_variants.h>

#include <entropy.h>

#include "camera.h"
#include "compute.h"
#include "gui.h"
#include "input_handling.h"
#include "manager.h"
#include "rendering.h"
#include "scene.h"
#include "settings.h"
#include "shader_c.h"

GLFWwindow *window;

int main(int argc, char *argv[]) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raytracing for fun and fun", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_click_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /*glEnable(GL_DEBUG_OUTPUT);*/
    /*glDebugMessageCallback(MessageCallback, 0);*/

    stbi_set_flip_vertically_on_load(1);

    /*glCullFace(GL_BACK);*/
    /*glEnable(GL_CULL_FACE);*/
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    int work_grp_cnt[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

    printf("max global (total) work group counts x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    int work_grp_size[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

    printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1],
           work_grp_size[2]);

    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("max local work group invocations %i\n", work_grp_inv);

    {
        uint64_t seeds[2];
        entropy_getbytes((void *)seeds, sizeof(seeds));
        pcg32_srandom(seeds[0], seeds[1]);
    }

    {
        gui_init();
        manager        = init_manager();
        Camera *camera = make_camera();
        Manager_set_camera(manager, camera);

        camera->camera_pos[0] = camera_pos[0];
        camera->camera_pos[1] = camera_pos[1];
        camera->camera_pos[2] = camera_pos[2];

        camera->pitch = camera_orientation[0];
        camera->yaw   = camera_orientation[1];
        camera->zoom  = camera_orientation[2];

        update_camera_target(camera, 0, 0);
        update_camera_position_matrix(camera);
    }

    init_scene();

    // Shaders
    Shader *shader = newShader("shaders/main.vert", "shaders/main.frag", NULL);
    Shader_use(shader);
    Shader_set_int(shader, "tex", 0);

    compute_t *compute_shader = build_compute_shader("shaders/raytracer.comp");
    compute_use(compute_shader);
    compute_set_float(compute_shader, "near_plane", near_plane);
    compute_set_float(compute_shader, "far_plane", far_plane);
    compute_set_int(compute_shader, "n_spheres", n_spheres);
    compute_set_int(compute_shader, "n_triangles", n_triangles);

    // Quad
    unsigned int VAO;
    unsigned int VBO;
    float        quadVertices[] = {
        // positions        // texture Coords
        -1.0f, +1.0f, 0.0f, 0.0f, 1.0f, //
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, //
        +1.0f, +1.0f, 0.0f, 1.0f, 1.0f, //
        +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, //
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    // SSBOs
    // Spheres
    set_shader_storage_buffer(10, n_spheres * sizeof(float) * 4, positions);
    set_shader_storage_buffer(11, n_spheres * sizeof(float), radius);
    set_shader_storage_buffer(12, n_spheres * sizeof(int), material_type);
    set_shader_storage_buffer(13, n_spheres * sizeof(float) * 4, albedo);
    set_shader_storage_buffer(14, n_spheres * sizeof(float) * 4, emission);
    set_shader_storage_buffer(15, n_spheres * sizeof(float), roughness);
    // Triangles
    set_shader_storage_buffer(20, n_triangles * sizeof(float) * 4, triangle_v0);
    set_shader_storage_buffer(21, n_triangles * sizeof(float) * 4, triangle_v1);
    set_shader_storage_buffer(22, n_triangles * sizeof(float) * 4, triangle_v2);
    set_shader_storage_buffer(23, n_triangles * sizeof(float) * 4, triangle_albedo);
    set_shader_storage_buffer(24, n_triangles * sizeof(float) * 4, triangle_emission);

    // Compute texture
    const unsigned int TEXTURE_WIDTH  = WINDOW_WIDTH;
    const unsigned int TEXTURE_HEIGHT = WINDOW_HEIGHT;

    glGenTextures(1, &manager->render_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, manager->render_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, manager->render_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, manager->render_texture);

    glGenTextures(1, &manager->debug_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, manager->debug_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(1, manager->debug_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

#if 0
    {
        // TODO(@h3nnn4n): Would be nice for this to be async to make it start rendering faster.
        // Although it would probably invalidate whatever has been rendered so far.
        // Alternatively we could have a skybox toggle and we lazily load the texture.
        printf("loading skybox image\n");
        int32_t        image_width, image_height, n_components;
        unsigned char *image_data =
            stbi_load("assets/cape_hill_half.png", &image_width, &image_height, &n_components, 0);
        printf("  loaded: resolution %dx%d with %d components\n", image_width, image_height, n_components);
        glGenTextures(1, &manager->skybox_texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, manager->skybox_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        glBindImageTexture(2, manager->skybox_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        stbi_image_free(image_data);
    }
#endif

    printf("starting render loop\n");

    while (!glfwWindowShouldClose(window)) {
        // Process input
        glfwPollEvents();
        process_input(window);

        // Timer
        Manager_tick_timer(manager);

        if (manager->frame_count % 1000 == 0) {
            printf("fps: %f\n", 1.0f / manager->delta_time);
        }

        // Run compute shader
        compute_use(compute_shader);
        compute_set_float(compute_shader, "time", manager->current_time);
        compute_set_matrix4(compute_shader, "camera_view", &manager->camera->view);
        compute_set_bool(compute_shader, "orthographic", manager->camera->orthographic);
        compute_set_bool(compute_shader, "incremental_rendering", manager->incremental_rendering);
        compute_set_int(compute_shader, "rng_seed", pcg32_random());

        compute_set_vec3(compute_shader, "look_from", &manager->camera->camera_pos);
        compute_set_vec3(compute_shader, "look_at", &manager->camera->camera_target);
        compute_set_float(compute_shader, "vfov", manager->camera->zoom);
        compute_set_float(compute_shader, "yaw", manager->camera->yaw);
        compute_set_float(compute_shader, "pitch", manager->camera->pitch);

        vec3 black = {0.0f, 0.0f, 0.0f};
        vec3 white = {1.0f, 1.0f, 1.0f};
        if (manager->ambient_light)
            compute_set_vec3(compute_shader, "ambient_light", &white);
        else
            compute_set_vec3(compute_shader, "ambient_light", &black);

        glDispatchCompute(TEXTURE_WIDTH / 32, TEXTURE_HEIGHT / 32, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Main pass
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Shader_use(shader);
        Shader_set_int(shader, "tone_mapping_mode", manager->tone_mapping_mode);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // Render gui
        gui_render();

        // Draw to screen
        glfwSwapBuffers(window);
    }

    gui_terminate();
    glfwTerminate();

    destroy_camera(manager->camera);

    return 0;
}
