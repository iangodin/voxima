
#include <utils/Huml.h>

int main()
{
	try
	{
		huml::Parser parser( R"huml(
meta::
  lang:: "en", "en-US", "en-GB"
)huml" );

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
