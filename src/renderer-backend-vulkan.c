/*
 * Copyright (C) 2016 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <wpe/renderer-backend-vulkan.h>

#include "loader-private.h"
#include "renderer-backend-vulkan-private.h"
#include <stdlib.h>

__attribute__((visibility("default")))
struct wpe_renderer_backend_vulkan*
wpe_renderer_backend_vulkan_create()
{
    struct wpe_renderer_backend_vulkan* backend = malloc(sizeof(struct wpe_renderer_backend_vulkan));
    if (!backend)
        return 0;

    backend->interface = wpe_load_object("_wpe_renderer_backend_vulkan_interface");
    backend->interface_data = backend->interface->create();

    return backend;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_vulkan_destroy(struct wpe_renderer_backend_vulkan* backend)
{
    backend->interface->destroy(backend->interface_data);
    backend->interface_data = 0;

    free(backend);
}

__attribute__((visibility("default")))
VkInstance
wpe_renderer_backend_vulkan_get_instance(struct wpe_renderer_backend_vulkan* backend)
{
    return backend->interface->get_instance(backend->interface_data);
}

__attribute__((visibility("default")))
struct wpe_renderer_backend_vulkan_target*
wpe_renderer_backend_vulkan_target_create(int host_fd)
{
    struct wpe_renderer_backend_vulkan_target* target = malloc(sizeof(struct wpe_renderer_backend_vulkan_target));
    if (!target)
        return 0;

    target->interface = wpe_load_object("_wpe_renderer_backend_vulkan_target_interface");
    target->interface_data = target->interface->create(target, host_fd);

    return target;
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_vulkan_target_destroy(struct wpe_renderer_backend_vulkan_target* target)
{
    target->interface->destroy(target->interface_data);
    target->interface_data = 0;

    // target->client = 0;
    // target->client_data = 0;

    free(target);
}

__attribute__((visibility("default")))
void
wpe_renderer_backend_vulkan_target_initialize(struct wpe_renderer_backend_vulkan_target* target, struct wpe_renderer_backend_vulkan* backend, uint32_t width, uint32_t height)
{
    target->interface->initialize(target->interface_data, backend->interface_data, width, height);
}
