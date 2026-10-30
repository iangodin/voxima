#pragma once

#include "Camera.h"
#include "Ease.h"
#include "EaseCubic.h"
#include "Scene.h"
#include "Stage.h"
#include "Tween.h"

#include <chrono>

// Where the camera looks: the center and zoom (see Camera)
struct CameraView
{
	Point center = { 0.F, 0.F };
	float zoom = 1.F;
};

inline CameraView view_of( const Camera &camera )
{
	return { camera.center, camera.zoom };
}

// The view a fraction t of the way from a to b. The zoom goes evenly in its logarithm (each second looks like the
// same amount of zooming, not most of it at the end), and the center follows so that the point both views leave
// where it is on the screen stays there: zooming in on something looks like moving in on it, whatever else is
// panned. For t beyond 0 to 1 (an ease that overshoots) it carries straight on.
CameraView view_between( const CameraView &a, const CameraView &b, float t );

// The view that fits `box` on the picture, with `margin` (a fraction of the box's size) around it, never closer than
// `max_zoom`. An empty box gives the current view.
CameraView view_framing( const Camera &camera, const AABB &box, float margin = 0.1F, float max_zoom = 100.F );

// Move the camera to a view over a length of time
Scene camera_to( Stage &stage, CameraView target, std::chrono::duration<double> duration, EaseFunction ease = ease_cubic_in_out );

// Move the camera to fit a box (in stage units) or a visual (one directly on the stage: its box in the stage's
// frame, position included)
Scene camera_frame( Stage &stage, AABB box, std::chrono::duration<double> duration, EaseFunction ease = ease_cubic_in_out, float margin = 0.1F );
Scene camera_frame( Stage &stage, const Visual &visual, std::chrono::duration<double> duration, EaseFunction ease = ease_cubic_in_out, float margin = 0.1F );
