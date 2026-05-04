
#include <utils/Huml.h>

int main()
{
	try
	{
		huml::Parser parser( R"huml(list:: "a", "b", "c")huml" );

		auto result = parser.parse();
		std::cout << result << '\n';
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
