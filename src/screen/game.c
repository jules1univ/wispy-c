#include "game.h"

void game_screen(w_state *state) {

  Texture block_textures[6] = {
      get_texture_by_id(state, "blocks\\grass.png"),
      get_texture_by_id(state, "blocks\\dirt.png"),
      get_texture_by_id(state, "blocks\\stone.png"),
      get_texture_by_id(state, "blocks\\mineral.png"),
      get_texture_by_id(state, "blocks\\mineral_or.png"),
      get_texture_by_id(state, "blocks\\mineral_iron.png")};

  Texture player_textures[4] = {
      get_texture_by_id(state, "player\\player_idle1.png"),
      get_texture_by_id(state, "player\\player_idle2.png"),
      get_texture_by_id(state, "player\\player_walk1.png"),
      get_texture_by_id(state, "player\\player_walk2.png"),
  };

  w_bridge *td = create_bridge();
  if (td == NULL)
    return;

  w_blockbreaker *bb = create_blockbreaker(state, td->chunk_view, td->camera);
  if (bb == NULL) {
    destroy_bridge(td);
    return;
  }
  while (!WindowShouldClose() && td->is_active) {
#ifndef __ANDROID__
    update_controls(td->ctrl);
#endif

    float dt = GetFrameTime();

    // TODO: make the player speed move faster if the player is far away from
    // the camera
    float speed = dt * PLAYER_SPEED;

#ifdef _WIN32
    if (TryEnterCriticalSection(&td->chunk_view->csec))
#else
    if (pthread_mutex_trylock(&td->chunk_view->mutex) == 0)
#endif // _WIN32
    {
      BeginTextureMode(state->render);
      ClearBackground(BLACK);
      DrawRectangleGradientV(0, 0, RENDER_W, RENDER_H,
                             (Color){66, 135, 245, 255},
                             (Color){142, 184, 250, 255});

      BeginMode2D(*(td->camera));
      smooth_vec(&td->camera->target, td->camera_target, speed);

      for (unsigned int i = 0; i < td->chunk_view->textures_len; i++) {
        DrawTexturePro(block_textures[td->chunk_view->blocks[i].block.type - 1],
                       td->chunk_view->blocks[i].src,
                       td->chunk_view->blocks[i].dst, VEC_ZERO, 0,
                       td->chunk_view->blocks[i].light);
      }
      w_breakstate bstate = update_blockbreaker(bb, td->player, dt);

      if (bstate == BS_BREAKING) {
        draw_blockbreaker(bb);
      } else if (bstate == BS_BROKEN) {
        td->force_update = true;
      }

      DrawTexturePro(player_textures[td->player->state], td->player->src,
                     (Rectangle){.x = td->player->position.x,
                                 .y = td->player->position.y,
                                 .width = td->player->dst.width,
                                 .height = td->player->dst.height},
                     VEC_ZERO, 0, WHITE);

      EndMode2D();
#ifdef __ANDROID__
      draw_controls(td->ctrl);
#endif
      DrawFPS(0, 0);

      EndTextureMode();

#ifdef _WIN32
      LeaveCriticalSection(&td->chunk_view->csec);
#else
      pthread_mutex_unlock(&td->chunk_view->mutex);
#endif // _WIN32
    }
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(state->render.texture, state->src_rnd, state->dest_rnd,
                   VEC_ZERO, 0.0f, WHITE);
    EndDrawing();
  }
  destroy_blockbreaker(bb);
  destroy_bridge(td);
}
