/*
 * factory.c -- the factory method interfaces
 * @author Kyle (ohyes@hk1229.cn)
 */

#include <framework/mlt.h>
#include <string.h>
#include <limits.h>

extern mlt_filter filter_extract_alpha_channel_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
    char file[ PATH_MAX ];
    snprintf( file, PATH_MAX, "%s/kai/%s", mlt_environment( "MLT_DATA" ), (char*) data );
    return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
    MLT_REGISTER( filter_type, "extract_alpha_channel", filter_extract_alpha_channel_init );

    MLT_REGISTER_METADATA( filter_type, "extract_alpha_channel", metadata, "filter_extract_alpha_channel.yml" );
}
