/*
 * filter_extract_alpha_channel.c
 * @author Kyle (ohyes@hk1229.cn)
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

    mlt_filter filter = mlt_frame_pop_service( frame );
    // mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
    // mlt_position position = mlt_filter_get_position( filter, frame );
    // mlt_position length = mlt_filter_get_length2( filter, frame );

    *format = mlt_image_rgb24a;

    // Get the image
    int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

    // Only process if we have no error.
    if ( error == 0 )
    {
        unsigned long num = *width * *height * 4;
        uint8_t *p = *image + 0;
        for (unsigned long i = 0; i < num; i += 4)
        {
            p[0 + i] = p[3 + i];
            p[1 + i] = p[3 + i];
            p[2 + i] = p[3 + i];
            p[3 + i] = 0xFF;
        }

    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
    mlt_frame_push_service( frame, filter );
    mlt_frame_push_get_image( frame, filter_get_image );

    return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_extract_alpha_channel_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
    mlt_filter filter = mlt_filter_new( );
    if ( filter != NULL )
    {
        filter->process = filter_process;
        mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "start", arg == NULL ? "1" : arg );
        mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "level", NULL );
    }
    return filter;
}

