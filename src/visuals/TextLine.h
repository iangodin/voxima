#pragma once

#include "Group.h"

class TextLine : public Group
{
public:
	void set_horizontal_baseline( float baseline )
	{
		mHorizontalBaseline = baseline;
	}

	[[nodiscard]] float get_horizontal_baseline() const
	{
		return mHorizontalBaseline;
	}

	void set_vertical_axis( float axis )
	{
		mVerticalAxis = axis;
	}

	[[nodiscard]] float get_vertical_axis() const
	{
		return mVerticalAxis;
	}

private:
	float mHorizontalBaseline = 0.0f;
	float mVerticalAxis = 0.0f;
};
