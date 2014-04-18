#include "vcioctl.h"
#include "vcdevice.h"
#include "debug.h"
#include "media/v4l2-dev.h"

extern const char * vc_driver_name;

int vcdev_querycap( struct file * file, void * priv,
                            struct v4l2_capability * cap)
{
    PRINT_DEBUG("IOCTL QueryCap");
    strcpy(cap->driver, vc_driver_name);
    strcpy(cap->card, vc_driver_name);
    cap->capabilities = V4L2_CAP_VIDEO_CAPTURE |
                        V4L2_CAP_STREAMING     |
                        V4L2_CAP_READWRITE;
    return 0;
}

int vcdev_enum_input( struct file * file, void * priv,
                              struct v4l2_input * inp)
{
    PRINT_DEBUG( "IOCTL enum_input(%u)\n",inp->index);
    
    if(inp->index >= 1 )
        return -EINVAL;
    
    inp->type = V4L2_INPUT_TYPE_CAMERA;
    //sprintf(inp->name,"vc_in %u", inp->index );
    return 0;
}

int vcdev_g_input( struct file * file, void * priv,
                           unsigned int * i )
{
   PRINT_DEBUG( "IOCTL get_input(%u)\n",*i);

   *i = 0;
   return 0; 
}

int vcdev_s_input( struct file * file, void * priv,
                           unsigned int i )
{
	PRINT_DEBUG( "IOCTL set_input(%u)\n", i );
    if ( i>= 1 )
        return -EINVAL;
    return 0;
}

int vcdev_enum_fmt_vid_cap( struct file * file, void * priv,
                                    struct v4l2_fmtdesc * f)
{
    struct vc_device * dev;

    PRINT_DEBUG( "IOCTL enum_fmt(%u)\n", f->index );

    dev = ( struct vc_device * ) video_drvdata(file);

    if( f->index >= dev->nr_fmts )
        return -EINVAL;

    strcpy(f->description,"RGB24 (LE)");
    f->pixelformat = V4L2_PIX_FMT_RGB24;
    return 0;
}
 
int vcdev_g_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    struct vc_device * dev;
    PRINT_DEBUG( "IOCTL get_fmt\n");

    dev = ( struct vc_device * ) video_drvdata(file);
    PRINT_DEBUG("Private at %0X\n",(unsigned int) dev);

    /*f->fmt.pix.width  = dev->v4l2_fmt[0]->width;
    f->fmt.pix.height = dev->v4l2_fmt[0]->height;
    f->fmt.pix.field  = V4L2_FIELD_INTERLACED;
    f->fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    f->fmt.pix.bytesperline = (dev->v4l2_fmt*24) >> 3;
    f->fmt.pix.sizeimage = 480*f->fmt.pix.bytesperline;
    f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;*/
    memcpy(&f->fmt.pix, dev->v4l2_fmt[0],
     sizeof( struct v4l2_pix_format) );
    
    return 0;
}

int vcdev_s_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    int ret;
    PRINT_DEBUG( "IOCTL set_fmt\n" );

    ret = vcdev_g_fmt_vid_cap( file, priv, f);
    if ( ret < 0 )
         return ret;
    return 0;
}