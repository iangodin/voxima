#include <themes/ThemeUtils.h>

namespace ThemeUtils
{

float get_theme_font_size( Scale scale )
{
	switch ( scale )
	{
		case Scale::TINY:
			return 0.1F;
		case Scale::SMALL:
			return 0.25F;
		case Scale::MEDIUM:
			return 0.3F;
		case Scale::LARGE:
			return 0.75F;
		case Scale::HUGE:
			return 1.5F;
		default:
			return 0.3F;
	}
}

FontParams get_font_params( const FontProperties &props )
{
	int weight = 400; // Regular
	if ( props.weight == Weight::LIGHT )
	{
		weight = 300;
	}
	if ( props.weight == Weight::BOLD )
	{
		weight = 700;
	}
	if ( props.weight == Weight::BLACK )
	{
		weight = 900;
	}

	int slant = ( props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE ) ? 1 : 0;

	return { weight, 100, slant };
}

std::string get_amiri_filename( const FontProperties &props )
{
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;

	if ( is_bold && is_italic )
	{
		return "Amiri-BoldItalic.ttf";
	}
	if ( is_bold )
	{
		return "Amiri-Bold.ttf";
	}
	if ( is_italic )
	{
		return "Amiri-Italic.ttf";
	}
	return "Amiri-Regular.ttf";
}

std::string get_fira_filename( const FontProperties &props, std::string_view base_name )
{
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;
	bool is_light = props.weight == Weight::LIGHT;

	if ( base_name == "FiraGO" )
	{
		if ( is_bold && is_italic )
		{
			return "FiraGO-BoldItalic.ttf";
		}
		if ( is_bold )
		{
			return "FiraGO-Bold.ttf";
		}
		if ( is_italic )
		{
			return "FiraGO-Italic.ttf";
		}
		if ( is_light )
		{
			return "FiraGO-Light.ttf";
		}
		return "FiraGO-Regular.ttf";
	}
	if ( base_name == "FiraMono" )
	{
		if ( is_bold )
		{
			return "FiraMono-Bold.ttf";
		}
		return "FiraMono-Regular.ttf";
	}
	return std::string( base_name ) + "-Regular.ttf";
}

std::string get_ibmplexsansarabic_filename( const FontProperties &props )
{
	Weight w = props.weight.value_or( Weight::REGULAR );
	if ( w == Weight::LIGHT )
	{
		return "IBMPlexSansArabic-Light.ttf";
	}
	if ( w == Weight::BOLD )
	{
		return "IBMPlexSansArabic-Bold.ttf";
	}
	if ( w == Weight::BLACK )
	{
		return "IBMPlexSansArabic-Bold.ttf";
	}
	return "IBMPlexSansArabic-Regular.ttf";
}

std::string get_ibmplexmono_filename( const FontProperties &props )
{
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;
	if ( is_bold && is_italic )
	{
		return "IBMPlexMono-BoldItalic.ttf";
	}
	if ( is_bold )
	{
		return "IBMPlexMono-Bold.ttf";
	}
	if ( is_italic )
	{
		return "IBMPlexMono-Italic.ttf";
	}
	return "IBMPlexMono-Regular.ttf";
}

std::string get_ibmplexsansjp_filename( const FontProperties &props )
{
	Weight w = props.weight.value_or( Weight::REGULAR );
	if ( w == Weight::LIGHT )
	{
		return "IBMPlexSansJP-Light.ttf";
	}
	if ( w == Weight::BOLD )
	{
		return "IBMPlexSansJP-Bold.ttf";
	}
	if ( w == Weight::BLACK )
	{
		return "IBMPlexSansJP-Bold.ttf";
	}
	return "IBMPlexSansJP-Regular.ttf";
}

std::string get_tajawal_filename( const FontProperties &props )
{
	Weight w = props.weight.value_or( Weight::REGULAR );
	if ( w == Weight::LIGHT )
	{
		return "Tajawal-Light.ttf";
	}
	if ( w == Weight::BOLD )
	{
		return "Tajawal-Bold.ttf";
	}
	if ( w == Weight::BLACK )
	{
		return "Tajawal-Black.ttf";
	}
	return "Tajawal-Regular.ttf";
}

std::string get_newcm_filename( const FontProperties &props, std::string_view base_name )
{
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;
	bool is_light = props.weight == Weight::LIGHT;

	Scale scale = props.scale.value_or( Scale::MEDIUM );
	bool is_small = scale == Scale::TINY || scale == Scale::SMALL;

	std::string family{ base_name };
	if ( base_name == "NewCM" || base_name == "NewCMSans" || base_name == "NewCMUncial" )
	{
		family += ( is_small ? "08" : "10" );
	}
	else if ( base_name == "NewCMMono" )
	{
		family += "10";
	}

	if ( family.starts_with( "NewCM08" ) || family.starts_with( "NewCM10" ) )
	{
		bool has_bold = family.starts_with( "NewCM10" );
		if ( has_bold && is_bold && is_italic )
		{
			return family + "-BoldItalic.otf";
		}
		if ( has_bold && is_bold )
		{
			return family + "-Bold.otf";
		}
		if ( is_light && is_italic )
		{
			return family + "-BookItalic.otf";
		}
		if ( is_light )
		{
			return family + "-Book.otf";
		}
		if ( is_italic )
		{
			return family + "-Italic.otf";
		}
		return family + "-Regular.otf";
	}
	if ( family.starts_with( "NewCMMono10" ) )
	{
		if ( is_bold && is_italic )
		{
			return "NewCMMono10-BoldOblique.otf";
		}
		if ( is_bold )
		{
			return "NewCMMono10-Bold.otf";
		}
		if ( is_light && is_italic )
		{
			return "NewCMMono10-BookItalic.otf";
		}
		if ( is_light )
		{
			return "NewCMMono10-Book.otf";
		}
		if ( is_italic )
		{
			return "NewCMMono10-Italic.otf";
		}
		return "NewCMMono10-Regular.otf";
	}
	if ( family.starts_with( "NewCMSans" ) )
	{
		bool has_bold = family.starts_with( "NewCMSans10" );
		if ( has_bold && is_bold && is_italic )
		{
			return family + "-BoldOblique.otf";
		}
		if ( has_bold && is_bold )
		{
			return family + "-Bold.otf";
		}
		if ( is_light && is_italic )
		{
			return family + "-BookOblique.otf";
		}
		if ( is_light )
		{
			return family + "-Book.otf";
		}
		if ( is_italic )
		{
			return family + "-Oblique.otf";
		}
		return family + "-Regular.otf";
	}
	if ( family.starts_with( "NewCMUncial" ) )
	{
		if ( is_bold )
		{
			return family + "-Bold.otf";
		}
		if ( is_light )
		{
			return family + "-Book.otf";
		}
		return family + "-Regular.otf";
	}
	if ( family == "NewCMMath" )
	{
		if ( is_bold )
		{
			return "NewCMMath-Bold.otf";
		}
		if ( is_light )
		{
			return "NewCMMath-Book.otf";
		}
		return "NewCMMath-Regular.otf";
	}
	if ( family == "NewCMSansMath" )
	{
		return "NewCMSansMath-Regular.otf";
	}
	return family + "-Regular.otf";
}

std::string get_devanagari_filename( const FontProperties &props )
{
	bool is_light = props.weight == Weight::LIGHT;

	Scale scale = props.scale.value_or( Scale::MEDIUM );
	bool is_small = scale == Scale::TINY || scale == Scale::SMALL;

	std::string family = is_small ? "NewCM08Devanagari" : "NewCM10Devanagari";

	if ( is_light )
	{
		return family + "-Book.otf";
	}
	return family + "-Regular.otf";
}

std::size_t get_cjk_face_index( const FontProperties &props )
{
	switch ( props.weight.value_or( Weight::REGULAR ) )
	{
		case Weight::LIGHT:
			return 5;
		case Weight::REGULAR:
			return 10;
		case Weight::BOLD:
			return 25;
		case Weight::BLACK:
			return 30;
		default:
			return 10;
	}
}

}
