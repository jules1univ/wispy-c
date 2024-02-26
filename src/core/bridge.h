#pragma once

#include "../stdafx.h"

#include "../entities/player.h"
#include "../terrain/block.h"
#include "../terrain/chunk.h"
#include "../terrain/chunk_group.h"
#include "../terrain/chunk_view.h"

#include "./utils/camera.h"
#include "controls.h"
#include "state.h"

typedef struct w_bridge {
  bool is_active;
  bool force_update;

  w_chunkgroup *chunk_group;
  w_chunkview *chunk_view;
  w_controls *ctrl;

  w_player *player;

  w_camera *camera;
  Vector2 camera_target;

#ifdef _WIN32
  HANDLE handle;
#else
  pthread_t handle;
#endif // _WIN32

} w_bridge;

w_bridge *create_bridge();
void destroy_bridge(w_bridge *td);
void physics_update(w_bridge *td);

#ifdef _WIN32
int WINAPI update_bridge(PVOID arg);
#else
void *update_bridge(void *arg);
#endif
