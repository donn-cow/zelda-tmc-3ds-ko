/*
 * Updater stub for builds configured with -DTMC3DS_UPDATER=OFF.
 *
 * Used by the Korean-patch build: the online updater would replace this build
 * with the official release (which cannot run the Korean ROM), and it needs
 * curl/jansson/mbedtls. Every request reports that updates are disabled.
 */
#include "updater.h"

#include <stdio.h>
#include <string.h>

void Updater_Init(const char* launch_path) {
    (void)launch_path;
}

void Updater_Check(void) {}

void Updater_SetChannel(bool prerelease) {
    (void)prerelease;
}

void Updater_Download(void) {}

void Updater_Cancel(void) {}

void Updater_GetStatus(UpdateStatus* out) {
    if (out == NULL)
        return;
    memset(out, 0, sizeof(*out));
    out->state = UPDATE_ERROR;
    snprintf(out->message, sizeof(out->message), "Updates disabled in this build");
}

bool Updater_Busy(void) {
    return false;
}

bool Updater_ShouldClose(void) {
    return false;
}

void Updater_Shutdown(void) {}

unsigned Updater_GetNotes(char* out, unsigned capacity) {
    if (out != NULL && capacity > 0)
        out[0] = '\0';
    return 0;
}

unsigned Update_FormatNotes(const char* markdown, char lines[][43], unsigned capacity) {
    (void)markdown;
    (void)lines;
    (void)capacity;
    return 0;
}
