/*
 * Copyright (C) 2015, 2016 Igalia S.L.
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

#include "loader-private.h"

#ifdef WIN32
#include <windows.h>
typedef HMODULE LibraryHandle;
#else
#include <dlfcn.h>
#include <stdarg.h>
typedef void * LibraryHandle;
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* s_impl_library = 0;
static struct wpe_loader_interface* s_impl_loader = 0;

#ifndef WPE_BACKEND
#define IMPL_LIBRARY_NAME_BUFFER_SIZE 512
static char* s_impl_library_name;
static char s_impl_library_name_buffer[IMPL_LIBRARY_NAME_BUFFER_SIZE];
#endif

#ifndef WPE_BACKEND
static void
wpe_loader_set_impl_library_name(const char* impl_library_name)
{
    size_t len;

    if (!impl_library_name)
        return;

    len = strlen(impl_library_name) + 1;
    if (len == 1)
        return;

    if (len > IMPL_LIBRARY_NAME_BUFFER_SIZE)
        s_impl_library_name = (char *)malloc(len);
    else
        s_impl_library_name = s_impl_library_name_buffer;
    memcpy(s_impl_library_name, impl_library_name, len);
}
#endif

void report_error(const char * fmt, ...)
{
    char msg[512];
    va_list valist;
    va_start(valist, fmt);
    size_t count = vsnprintf(msg, sizeof(msg), fmt, valist);
    fwrite(msg, sizeof(char), count, stderr);
#ifdef WIN32
    // cause nobody pays attention to stderr on windows...
    MessageBoxA(NULL, msg, "Error", MB_ICONERROR | MB_OK);
#endif
    va_end(valist);
}

#ifdef WIN32
void report_last_error(const char * message)
{
    //https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror
    DWORD errorMessageID = GetLastError();
    if (errorMessageID) {
        LPSTR messageBuffer = NULL;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        report_error("wpe: %s: %s\n", message, messageBuffer);
        LocalFree(messageBuffer);
    }
}
#endif

LibraryHandle openlibrary(const char * library_name, const char * context)
{
#ifdef WIN32
    LibraryHandle handle = LoadLibraryA(library_name);
    if (!handle) {
        char message[256];
        sprintf(message, "Failed to load library %s (%s)", library_name, context);
        report_last_error(message);
		abort();
    }
#else
    LibraryHandle handle = dlopen(library_name, RTLD_NOW);
    if (!handle) {
        report_error("wpe: could not load %s: %s\n", context, dlerror());
		abort();
    }
#endif
    return handle;
}

void * loadsymbol(LibraryHandle library, const char * symbol)
{
#ifdef WIN32
    void * ret = GetProcAddress(library, symbol);
    if (!ret) {
        char message[256];
        sprintf(message, "Failed to load symbol %s", symbol);
        report_last_error(message);
    }
    return ret;
#else
    return dlsym(library, symbol);
#endif
}

void
load_impl_library()
{
#ifdef WPE_BACKEND
    s_impl_library = openlibrary(WPE_BACKEND, "compile-time defined WPE_BACKEND");
#else
#ifndef NDEBUG
    // Get the impl library from an environment variable, if available.
    char* env_library_name = getenv("WPE_BACKEND_LIBRARY");
    if (env_library_name) {
        char message[256];
        snprintf(message, sizeof(message), "specified WPE_BACKEND_LIBRARY(%s)", env_library_name);
        s_impl_library = openlibrary(env_library_name, message);
        wpe_loader_set_impl_library_name(env_library_name);
    }
#endif
    if (!s_impl_library) {
        // Load libWPEBackend-default.so by ... default.
#ifdef WIN32
        static const char library_name[] = "WPEBackend-default.dll";
#else
        static const char library_name[] = "libWPEBackend-default.so";
#endif

        s_impl_library = openlibrary(library_name, "Default backend library");
		wpe_loader_set_impl_library_name(library_name);
    }
#endif
    if (!s_impl_library) {
        abort();
    }

    s_impl_loader = loadsymbol(s_impl_library, "_wpe_loader_interface");
}

bool
wpe_loader_init(const char* impl_library_name)
{
#ifndef WPE_BACKEND
    if (!impl_library_name) {
        fprintf(stderr, "wpe_loader_init: invalid implementation library name\n");
        abort();
    }

    if (s_impl_library) {
        if (!s_impl_library_name || strcmp(s_impl_library_name, impl_library_name) != 0) {
            fprintf(stderr, "wpe_loader_init: already initialized\n");
            return false;
        }
        return true;
    }

    s_impl_library = openlibrary(impl_library_name, "interface loaded library (wpe_loader_init)");
    if (!s_impl_library) {
        return false;
    }
    wpe_loader_set_impl_library_name(impl_library_name);

    s_impl_loader = loadsymbol(s_impl_library, "_wpe_loader_interface");
    return true;
#else
    return false;
#endif
}

const char*
wpe_loader_get_loaded_implementation_library_name(void)
{
#ifdef WPE_BACKEND
    return s_impl_library ? WPE_BACKEND : NULL;
#else
    return s_impl_library_name;
#endif
}

void*
wpe_load_object(const char* object_name)
{
    if (!s_impl_library)
        load_impl_library();

    if (s_impl_loader) {
        if (!s_impl_loader->load_object) {
            fprintf(stderr, "wpe_load_object: failed to load object with name '%s': backend doesn't implement load_object vfunc\n", object_name);
            abort();
        }
        return s_impl_loader->load_object(object_name);
    }

    void* object = loadsymbol(s_impl_library, object_name);
    if (!object)
        report_error("wpe_load_object: failed to load object with name '%s'\n", object_name);

    return object;
}
