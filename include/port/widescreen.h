#ifndef PORT_WIDESCREEN_H
#define PORT_WIDESCREEN_H

#include "dolphin/mtx.h"
#ifdef TARGET_PC
#include "port/netplay_runtime.h"
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool PartyBoard_WidescreenEnabled(void);
float PartyBoard_WidescreenAspectScale(void);
float PartyBoard_WidescreenPerspectiveAspect(float originalAspect);
void PartyBoard_WidescreenOrthoBounds(float originalLeft, float originalRight,
                                      float *renderLeft, float *renderRight);
/* How far HUD x moves towards the edges of the widened picture. */
float PartyBoard_WidescreenHudShift(float hudX);
/* Whether a sprite this size covers the original picture: a backdrop. */
bool PartyBoard_WidescreenBackdrop(float width, float height);
/* Widens a backdrop to the whole window. */
void PartyBoard_WidescreenStretchHudMatrix(Mtx matrix);
/* HUD x (0..HU_DISP_WIDTH) over a point of the widened 3D picture, and back. */
float PartyBoard_WidescreenHudX(float pictureX);
float PartyBoard_WidescreenPictureX(float hudX);

#ifdef __cplusplus
}
#endif

/* GX projections use the local window. Screen coordinates returned to game code
 * can drive input, AI and object positions, so peers use the original geometry. */
#ifdef TARGET_PC
#define PARTYBOARD_RENDER_ASPECT(value) PartyBoard_WidescreenPerspectiveAspect(value)
#define PARTYBOARD_GAME_ASPECT(value) (PartyBoard_NetplayEnabled() ? (value) : PartyBoard_WidescreenPerspectiveAspect(value))
#define PARTYBOARD_GAME_HUD_X(value) (PartyBoard_NetplayEnabled() ? (value) : PartyBoard_WidescreenHudX(value))
#define PARTYBOARD_GAME_PICTURE_X(value) (PartyBoard_NetplayEnabled() ? (value) : PartyBoard_WidescreenPictureX(value))
#else
#define PARTYBOARD_RENDER_ASPECT(value) (value)
#define PARTYBOARD_GAME_ASPECT(value) (value)
#define PARTYBOARD_GAME_HUD_X(value) (value)
#define PARTYBOARD_GAME_PICTURE_X(value) (value)
#endif

#endif
