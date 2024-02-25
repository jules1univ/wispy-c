#include "chunk_view.h"

w_chunkview *create_chunkview(w_chunk *current) {
  w_chunkview *chunk_view = malloc(sizeof(w_chunkview));
  if (chunk_view == NULL) {
    LOG("failed to allocate memory for chunk view");
    return NULL;
  }
  chunk_view = memset(chunk_view, 0, sizeof(w_chunkview));

  chunk_view->blocks = malloc(chunk_view->textures_len);
  if (chunk_view->blocks == NULL) {
    LOG("failed to allocate memory for chunk view blocks");
    free(chunk_view);
    return NULL;
  }
  chunk_view->target = current;

#ifdef _WIN32
  InitializeCriticalSection(&chunk_view->csec);
#else
  if (pthread_mutex_init(&chunk_view->mutex, NULL) != 0) {
    free(chunk_view->blocks);
    free(chunk_view);

    LOG("failed to create mutex for chunk view");
    return NULL;
  }
#endif // _WIN32

  LOG("creating chunk view");
  return chunk_view;
}

void destroy_chunkview(w_chunkview *chunk_view) {
  if (chunk_view == NULL) {
    LOG("chunk view (null) already destroyed");
    return;
  }
  LOG("destroying chunk view");
#ifdef _WIN32
  DeleteCriticalSection(&chunk_view->csec);
#else
  if (pthread_mutex_destroy(&chunk_view->mutex) != 0) {
    LOG("failed to close mutex (chunk view)");
  }
#endif // _WIN32
  if (chunk_view->blocks != NULL && chunk_view->textures_len > 0) {
    free(chunk_view->blocks);
  }
  free(chunk_view);
}

void update_renderblock_async(w_chunkview *chunk_view, w_renderblock *blocks,
                              size_t textures_len) {
#ifdef _WIN32
  EnterCriticalSection(&chunk_view->csec);
#else
  pthread_mutex_lock(&chunk_view->mutex);
#endif // _WIN32

  sfree(chunk_view->blocks);
  chunk_view->textures_len = textures_len;
  chunk_view->blocks = blocks;

#ifdef _WIN32
  LeaveCriticalSection(&chunk_view->csec);
#else
  pthread_mutex_unlock(&chunk_view->mutex);
#endif // _WIN32
}

bool update_chunkview(w_chunkview *chunk_view, w_chunkgroup *grp,
                      Rectangle view) {
  if (view.x < 0 || view.y < 0) {
    update_renderblock_async(chunk_view, NULL, 0);
    return true;
  }

  // TODO: fix float is not exact for x position
  unsigned int position = floorf(view.x / FULL_CHUNK_W);
  if (position != chunk_view->target->position) {
    int group_move = need_chunkgroup_update(grp, position);
    if (group_move > 0) {
      next_chunkgroup(grp);
    } else if (group_move < 0) {
      prev_chunkgroup(grp);
    }

    chunk_view->target = get_chunkgroup_chunk(grp, position);
    if (chunk_view->target == NULL) {
      return false;
    }
  }

  unsigned int position_next = round((view.x + view.width) / FULL_CHUNK_W);
  if (position_next != position) {
    chunk_view->next = get_chunkgroup_chunk(grp, position_next);
  } else {
    chunk_view->next = NULL;
  }

  size_t render_count = 0;
  size_t textures_len =
      CHUNK_W * CHUNK_H * ((chunk_view->next == NULL) ? 1 : 2);
  w_renderblock *blocks = malloc(textures_len * sizeof(w_renderblock));
  if (blocks == NULL) {
    return false;
  }
  memset(blocks, 0, textures_len * sizeof(w_renderblock));
  filter_chunkview_blocks(chunk_view->target, view, blocks, &render_count);
  if (chunk_view->next != NULL) {
    filter_chunkview_blocks(chunk_view->next, view, blocks, &render_count);
  }

  if (render_count == 0) {
    free(blocks);
    update_renderblock_async(chunk_view, NULL, 0);
    return true;
  } else if (render_count != textures_len) {
    w_renderblock *tmp = realloc(blocks, sizeof(w_renderblock) * render_count);
    if (tmp == NULL) {
      free(blocks);
      return false;
    }
    blocks = tmp;
  }

  update_renderblock_async(chunk_view, blocks, render_count);
  return true;
}

void filter_chunkview_blocks(w_chunk *chunk, Rectangle view,
                             w_renderblock *blocks, size_t *rendercount) {

  view.x -= RENDER_CUBE_GAP * CUBE_W;
  view.y -= RENDER_CUBE_GAP * CUBE_H;
  view.width += RENDER_CUBE_GAP * CUBE_W * 2;
  view.height += RENDER_CUBE_GAP * CUBE_H * 2;

  Rectangle block = {.x = 0, .y = 0, .width = CUBE_W, .height = CUBE_H};

#pragma omp parallel for
  for (unsigned int x = 0; x < CHUNK_W; x++) {

    bool noclear = false;
    for (unsigned int y = 0; y < CHUNK_H; y++) {

      unsigned int index = y * CHUNK_W + x;
      if (index >= CHUNK_W * CHUNK_H) {
        break;
      }
      if (chunk->blocks[index].type == BLOCK_AIR) {
        continue;
      } else if (!noclear) {
        if (chunk->blocks[index].is_background) {
          chunk->blocks[index].type = BLOCK_AIR;
          continue;
        }
        noclear = true;
      }

      block.x = (chunk->position * FULL_CHUNK_W) + (x * CUBE_W);
      block.y = y * CUBE_H;

      if (CheckCollisionRecs(block, view)) {
#pragma omp critical
        {
          blocks[*rendercount] = (w_renderblock){.dst = block,
                                                 .src = CUBE_SRC_RECT,
                                                 .light = WHITE,
                                                 .block = chunk->blocks[index]};
          (*rendercount)++;
        }
      }
    }
  }
}

void update_chunkview_lighting(w_chunkview *chunk_view, Vector2 light,
                               float radius) {
  if (chunk_view->blocks == NULL) {
    return;
  }

#pragma omp parallel for
  for (size_t i = 0; i < chunk_view->textures_len; i++) {
    Vector2 block_center = {chunk_view->blocks[i].dst.x + (CUBE_W / 2),
                            chunk_view->blocks[i].dst.y + (CUBE_H / 2)};

    float percent = 1.f - Vector2Distance(light, block_center) / radius;
    if (chunk_view->blocks[i].block.is_background) {
      percent -= 0.7;
    }
    percent = Clamp(percent, 0.f, 1.f);
    chunk_view->blocks[i].light = (Color){.r = (unsigned char)(255 * percent),
                                          .g = (unsigned char)(255 * percent),
                                          .b = (unsigned char)(255 * percent),
                                          .a = 255};
  }
}

w_block *get_chunkview_block(w_chunkview *chunk_view, Vector2 position) {
  unsigned int chunk_x = position.x / FULL_CHUNK_W;

  if (!(chunk_view->target->position == chunk_x ||
        (chunk_view->next != NULL && chunk_view->next->position == chunk_x))) {
    return NULL;
  }

  w_chunk *chunk = (chunk_view->target->position == chunk_x)
                       ? chunk_view->target
                       : chunk_view->next;
  unsigned int block_x = (int)(position.x / CUBE_W) - chunk->position * CHUNK_W;
  unsigned int block_y = (int)(position.y / CUBE_H);

  if (block_x + block_y * CHUNK_W > CHUNK_W * CHUNK_H) {
    return NULL;
  }
  return &(chunk->blocks[block_x + block_y * CHUNK_W]);
}
