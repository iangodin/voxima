
#include "Stage.h"
#include "Screenplay.h"
#include "Producer.h"

#include "MyMovie.h"

int main()
{
	Stage stage;

	// 1. Create the narrative
	auto movie = my_movie( stage );

	// 2. Hand it to the Producer
	Producer producer( std::move(movie), stage );

	// 3. Run until 'tick' returns false
	while ( producer.tick() )
	{
		std::cout << producer.current_frame() << ": " << stage.message << '\n';

		if ( producer.current_frame() == 100 )
			break;
	}

	return 0;
}

