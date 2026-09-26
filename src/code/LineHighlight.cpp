#include "LineHighlight.h"

#include <core/Stage.h>
#include <core/Theme.h>
#include <core/Tween.h>
#include <visuals/Rectangle.h>

#include <algorithm>

Lines &Lines::add( size_t first, size_t last )
{
	if ( first > last )
	{
		std::swap( first, last );
	}
	if ( last == 0 )
	{
		return *this; // there is no line 0
	}
	first = std::max<size_t>( first, 1 );

	// In order, with the runs that touch or overlap joined
	_runs.emplace_back( first, last );
	std::sort( _runs.begin(), _runs.end() );
	std::vector<std::pair<size_t, size_t>> merged;
	for ( const auto &run: _runs )
	{
		if ( !merged.empty() && run.first <= merged.back().second + 1 )
		{
			merged.back().second = std::max( merged.back().second, run.second );
		}
		else
		{
			merged.push_back( run );
		}
	}
	_runs = std::move( merged );
	return *this;
}

bool Lines::contains( size_t line ) const
{
	return std::any_of( _runs.begin(), _runs.end(), [line]( const auto &run ) { return run.first <= line && line <= run.second; } );
}

LineHighlight::LineHighlight( CodeListing &listing, const Theme &theme, const HighlightOptions &options ) :
	_listing( listing ),
	_options( options ),
	_band_color( options.band.value_or( theme.code_palette().line_highlight ) )
{
	set( Lines() );
}

std::vector<std::pair<size_t, size_t>> LineHighlight::clipped( const Lines &lines, size_t count )
{
	std::vector<std::pair<size_t, size_t>> out;
	for ( auto run: lines.runs() )
	{
		if ( run.first > count )
		{
			continue;
		}
		run.second = std::min( run.second, count );
		out.push_back( run );
	}
	return out;
}

LineHighlight::Band LineHighlight::band_of( std::pair<size_t, size_t> run ) const
{
	// A line's box: centered on the middle of its lowercase letters, and a line's pitch tall, so the boxes of
	// neighbouring lines meet and a run is one band
	const size_t count = _listing.line_count();
	const size_t first = std::clamp<size_t>( run.first, 1, std::max<size_t>( count, 1 ) ) - 1;
	const size_t last = std::clamp<size_t>( run.second, 1, std::max<size_t>( count, 1 ) ) - 1;
	const float half = _listing.line_pitch / 2.F;
	const float lift = 0.25F * _listing.em;
	return { _listing.line( first ).baseline + lift + half, _listing.line( last ).baseline + lift - half, 1.F };
}

void LineHighlight::blend( const Lines &a, const Lines &b, float t )
{
	_at = t < 0.5F ? a : b;
	const size_t count = _listing.line_count();
	if ( count == 0 )
	{
		return;
	}

	// Only the lines the listing has count: a range going past the end stops at it, and one wholly beyond it is not
	// there (so, alone, it points at nothing, and nothing is dimmed)
	const auto ra = clipped( a, count );
	const auto rb = clipped( b, count );

	// The text: a line is pointed out as much as it is in a, moving to how much it is in b; with no lines at all,
	// every line counts as pointed out, so nothing is dimmed
	auto pointed = [&]( const std::vector<std::pair<size_t, size_t>> &runs, size_t line )
	{
		return runs.empty() || std::any_of( runs.begin(), runs.end(), [line]( const auto &r ) { return r.first <= line && line <= r.second; } ) ? 1.F : 0.F;
	};
	for ( size_t i = 0; i < count; ++i )
	{
		const float from = pointed( ra, i + 1 );
		const float to = pointed( rb, i + 1 );
		const float focus = from + ( to - from ) * t;
		_listing.line( i ).color.a = _options.dim + ( 1.F - _options.dim ) * focus;
	}

	if ( !_options.bar )
	{
		return;
	}

	// The bands: the runs of a go to the runs of b, in order; one with nothing to go to fades out, or in
	const size_t bands = std::max( ra.size(), rb.size() );
	const float pad = _options.padding * _listing.em;
	const float left = _listing.left - pad;
	const float right = _listing.right + pad;

	while ( _bands.size() < bands )
	{
		auto rectangle = std::make_unique<Rectangle>();
		_bands.push_back( &_listing.background->add( std::move( rectangle ) ) );
	}

	for ( size_t k = 0; k < _bands.size(); ++k )
	{
		Band now;
		if ( k < ra.size() && k < rb.size() )
		{
			const Band x = band_of( ra[k] );
			const Band y = band_of( rb[k] );
			now = { x.top + ( y.top - x.top ) * t, x.bottom + ( y.bottom - x.bottom ) * t, 1.F };
		}
		else if ( k < ra.size() )
		{
			now = band_of( ra[k] );
			now.alpha = 1.F - t;
		}
		else if ( k < rb.size() )
		{
			now = band_of( rb[k] );
			now.alpha = t;
		}
		else
		{
			now.alpha = 0.F;
		}

		Rectangle &band = *_bands[k];
		band.position = { ( left + right ) / 2.F, ( now.top + now.bottom ) / 2.F };
		band.half_width = ( right - left ) / 2.F;
		band.half_height = std::max( 0.F, ( now.top - now.bottom ) / 2.F );
		band.color = _band_color;
		band.color.a = _band_color.a * std::clamp( now.alpha, 0.F, 1.F );
	}
}

Scene LineHighlight::to( const Stage &stage, Lines lines, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Lines from = _at;
	co_await tween( stage, duration, ease, [this, from, lines]( float t ) { blend( from, lines, t ); } );
	set( lines );
}
