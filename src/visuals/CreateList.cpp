#include "visuals/CreateList.h"

#include "visuals/CreateText.h"
#include "visuals/Placement.h"
#include "visuals/WrapText.h"

#include <core/Theme.h>

#include <algorithm>

Group &ItemList::row( size_t index ) const
{
	return dynamic_cast<Group &>( **( begin() + static_cast<std::ptrdiff_t>( index ) ) );
}

std::unique_ptr<ItemList> make_list( Theme &theme, const std::vector<std::string> &items, const ListOptions &options )
{
	auto list = std::make_unique<ItemList>();
	const float em = theme.get_stack( options.style ).font( 0 ).scale;
	constexpr float LINE = 1.2F; // a line's height in ems: what make_text lays lines out at

	// The markers, made first: the text starts after the widest, so it all lines up
	std::vector<std::unique_ptr<Group>> markers;
	float marker_width = 0.F;
	for ( size_t i = 0; i < items.size(); ++i )
	{
		markers.push_back( make_text( theme, options.numbered ? std::to_string( options.first_number + i ) + "." : options.bullet, options.style ) );
		const AABB box = box_of( *markers.back() );
		marker_width = std::max( marker_width, box.max.x() - box.min.x() );
	}

	float baseline = 0.F; // how far down the next item's first baseline is
	for ( size_t i = 0; i < items.size(); ++i )
	{
		auto row = std::make_unique<Group>();

		// A number is at the right of the marker column (so 9. and 10. line up on their dots); a bullet at the left
		auto &marker = row->add( std::move( markers[i] ) );
		const AABB marker_box = box_of( marker );
		const float marker_left = options.numbered ? marker_width - ( marker_box.max.x() - marker_box.min.x() ) : 0.F;
		marker.position = marker.position + Point{ marker_left - marker_box.min.x(), 0.F };

		// The text on the same baseline, after the markers
		std::string words = items[i];
		if ( options.max_width > 0.F )
		{
			words = wrap_lines( theme, words, options.style, std::max( options.max_width - marker_width - options.marker_gap * em, em ) );
		}
		auto &text = row->add( make_text( theme, words, options.style ) );
		text.position = text.position + Point{ marker_width + options.marker_gap * em - box_of( text ).min.x(), 0.F };

		// Down by a fixed step: the lines of the items before, and the space between
		row->position = { 0.F, -baseline };
		const size_t lines = static_cast<size_t>( std::count( words.begin(), words.end(), '\n' ) ) + 1;
		baseline += ( static_cast<float>( lines ) * LINE + options.spacing ) * em;

		list->add( std::move( row ) );
	}

	// The middle of the list at the origin (the rows keep their steps)
	AABB covered;
	for ( const auto &row: *list )
	{
		covered.add( box_of( *row ) );
	}
	if ( !covered.empty() )
	{
		const Point middle{ ( covered.min.x() + covered.max.x() ) / 2.F, ( covered.min.y() + covered.max.y() ) / 2.F };
		for ( const auto &row: *list )
		{
			row->position = row->position - middle;
		}
	}
	return list;
}
