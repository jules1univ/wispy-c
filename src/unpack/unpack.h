#pragma once
#include "../resource.h"
#include "../stdafx.h"

#define MAX_UNCOMPRESSE_TRY 4

typedef struct w_asset {
  char *name;

  char *buffer;
  unsigned int size;
} w_asset;

#ifdef _WIN32
char *load_resource(HINSTANCE hInstance, size_t *size);
w_asset *unpack_assets(HINSTANCE hInstance, size_t *size);
#else
char *load_resource(size_t *size);
w_asset *unpack_assets(size_t *size);
#endif