
#pragam once

#include "Group.h"

class TextLine : public Group
{
public:
	void set_baseline( float baseline )
	{
		mBaseline = baseline;
	}

	[[discard]] float get_baseline() const
	{
		return mBaseline;
	}

private:
	float mBaseline;
};

