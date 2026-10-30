#pragma once

#include "visuals/Group.h"

#include <fonts/FontStyle.h>

#include <memory>
#include <string>
#include <vector>

class Theme;

struct ListOptions
{
	FontStyle style = FontStyle{ Intent::PROSE };

	// Numbered (1. 2. 3.) instead of bulleted
	bool numbered = false;
	size_t first_number = 1;

	// What marks an item, when it is not numbered
	std::string bullet = "•";

	// Between the items (the space between one item's lines and the next's) and between the marker and the text, in ems: the
	// size of the text, so the list keeps its look at any size. The items go down a fixed step (their lines' height and this),
	// not by how tall each one's letters happen to be, so they are evenly spaced.
	float spacing = 0.45F;
	float marker_gap = 0.6F;

	// The most the list may be wide (stage units, the markers and their gap included): an item longer than fits is wrapped
	// onto more lines, each further line under the text, not the marker. 0 for no limit.
	float max_width = 0.F;
};

// A list: one row for each item (a marker and its text, the text in a column of its own, the markers lined up), the rows one
// under another. The rows are the children in order (row( i )), so each can be shown, lit or moved on its own; each row is
// a Group of the marker and the text. The origin is the middle of the list. An item can be several lines ("a\nb").
//
//     auto &list = stage.visuals.add( make_list( *stage.theme, { "first", "second", "third" } ) );
class ItemList : public Group
{
public:
	Group &row( size_t index ) const;
	size_t count() const
	{
		return size();
	}
};

std::unique_ptr<ItemList> make_list( Theme &theme, const std::vector<std::string> &items, const ListOptions &options = {} );
