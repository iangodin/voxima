#pragma once

#include "visuals/Connector.h"
#include "visuals/Group.h"
#include "visuals/Panel.h"
#include "visuals/Rectangle.h"

#include <memory>

struct CalloutOptions
{
	PanelOptions panel = {};

	// The pointer from the panel to what it is about: how it looks, and how close it comes to it
	ConnectorOptions pointer = { .gap = 0.15F, .thickness = 0.04F };
};

// A panel with a pointer to something: a note on a part of a picture, a label with an arrow. Place the panel where the
// note goes (panel(): next_to it, move_to it ...), then update() to point at the target from there; or follow() it to have it
// point at the target as either moves. The target is in the frame the callout is in (children of the same group, or of the
// stage), and the callout itself is best left at the origin of that frame.
//
//     auto &note = stage.visuals.add( make_callout( make_text( theme, "the peak", style ), peak ) );
//     next_to( note.panel(), peak, Anchor::TOP, 1.2F );
//     note.update();
class Callout : public Group
{
public:
	Callout( std::unique_ptr<Visual> content, const Visual &target, const CalloutOptions &options );

	Panel &panel()
	{
		return *_panel;
	}

	const Connector &pointer() const
	{
		return *_pointer;
	}

	// Point again: from the panel as it is now, to the target as it is now
	void update();

private:
	const Visual *_target;
	Panel *_panel;
	Connector *_pointer;
	std::unique_ptr<Rectangle> _stand_in; // the target's box, in this group's frame
};

std::unique_ptr<Callout> make_callout( std::unique_ptr<Visual> content, const Visual &target, const CalloutOptions &options = {} );

// Keep a callout pointing at its target at every frame, until the handle goes
Updaters::Handle follow( Stage &stage, Callout &callout );
