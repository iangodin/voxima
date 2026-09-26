#include "CameraMove.h"

#include <visuals/Layout.h>

#include <algorithm>
#include <cmath>

CameraView view_between( const CameraView &a, const CameraView &b, float t )
{
	const float za = std::max( a.zoom, 1e-6F );
	const float zb = std::max( b.zoom, 1e-6F );
	const float z = za * std::exp( std::log( zb / za ) * t );

	// How far along the pan we are: the fraction of the way the size of the view has come, so the one point on
	// which the two views agree holds still on the screen. With no zoom in it, just t.
	float s = t;
	if ( std::abs( zb - za ) > 1e-6F * std::max( za, zb ) )
	{
		s = ( 1.F / z - 1.F / za ) / ( 1.F / zb - 1.F / za );
	}
	return { a.center + ( b.center - a.center ) * s, z };
}

CameraView view_framing( const Camera &camera, const AABB &box, float margin, float max_zoom )
{
	if ( box.empty() )
	{
		return view_of( camera );
	}

	const float width = std::max( box.width() * ( 1.F + 2.F * margin ), 1e-6F );
	const float height = std::max( box.height() * ( 1.F + 2.F * margin ), 1e-6F );
	const float aspect = static_cast<float>( camera.height ) / static_cast<float>( camera.width );

	// The picture is BASE_VIEW_WIDTH / zoom across, and that times the aspect up: whichever way the box is tighter
	const float zoom = std::min( Camera::BASE_VIEW_WIDTH / width, Camera::BASE_VIEW_WIDTH * aspect / height );
	return { { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F }, std::min( zoom, max_zoom ) };
}

Scene camera_to( Stage &stage, CameraView target, std::chrono::duration<double> duration, EaseFunction ease )
{
	const CameraView from = view_of( stage.camera );
	co_await tween(
		stage,
		duration,
		ease,
		[&stage, from, target]( float t )
		{
			const CameraView view = view_between( from, target, t );
			stage.camera.center = view.center;
			stage.camera.zoom = view.zoom;
		} );

	// Exactly there, whatever the ease did at the end
	stage.camera.center = target.center;
	stage.camera.zoom = target.zoom;
}

Scene camera_frame( Stage &stage, AABB box, std::chrono::duration<double> duration, EaseFunction ease, float margin )
{
	co_await camera_to( stage, view_framing( stage.camera, box, margin ), duration, std::move( ease ) );
}

Scene camera_frame( Stage &stage, const Visual &visual, std::chrono::duration<double> duration, EaseFunction ease, float margin )
{
	co_await camera_frame( stage, layout_extent( visual ), duration, std::move( ease ), margin );
}
