#pragma once

#include "Narrator.h"

#include <utils/ObserverPtr.h>

#include <vector>

struct Stage;

class Narration
{
public:
	void setup( std::string_view name, Script &script, DubbingStage &stage );

	template<typename ...Keys>
	double speech( Keys &&...keys )
	{
		double duration = 0.0;
		for ( auto &narrator: _narrators )
		{
			auto text = _script->fetch( narrator.language(), std::forward<Keys>( keys )... ).as_multiline();
			duration = std::max( duration, narrator.speech( text ) );
		}

		return duration;
	}

	void speak( void );
	bool is_done() const;

private:
	size_t _audio_block = 0;
	ObserverPtr<Script> _script;
	std::vector<Narrator> _narrators;
};

