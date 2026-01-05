#pragma once

#include <stdbool.h>
#include <stddef.h>

bool SD_Mount(void);
bool SD_ReadFile(const char *path, char *out, size_t len);
bool SD_DeleteFile(const char *path);
