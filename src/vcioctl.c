#include <linux/videodev2.h>
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
    struct vc_device_format * fmt;
    int idx;

    PRINT_DEBUG( "IOCTL enum_fmt(%u)\n", f->index );

    dev = ( struct vc_device * ) video_drvdata(file);
    idx = f->index;

    if( idx >= dev->nr_fmts )
        return -EINVAL;

    fmt = &dev->out_fmts[ idx ];
    strcpy( f->description, fmt->name );
    f->pixelformat = fmt->fourcc;
    return 0;
}
 
int vcdev_g_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    struct vc_device * dev;
    PRINT_DEBUG( "IOCTL get_fmt\n");

    dev = ( struct vc_device * ) video_drvdata(file);

    memcpy(&f->fmt.pix, &dev->output_format,
     sizeof( struct v4l2_pix_format) );
    
    return 0;
}

int vcdev_try_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    struct vc_device * dev;
    #ifdef DEBUG
        char str[5];
        memcpy((void *)str,(void *)&f->fmt.pix.pixelformat, 4);
        str[4] = 0;
        PRINT_DEBUG( "IOCTL try_fmt(%dx%d), fmt %s\n", f->fmt.pix.width, f->fmt.pix.height, str);
    #endif

    dev = ( struct vc_device * ) video_drvdata( file );
    

     if( !check_supported_pixfmt( dev, f->fmt.pix.pixelformat ) ){
            f->fmt.pix.pixelformat = dev->output_format.pixelformat;
            PRINT_DEBUG("Unsupported\n");
     }

     if( !dev->conv_res_on ){
            f->fmt.pix.width = dev->output_format.width;
            f->fmt.pix.height = dev->output_format.height;
     }

     f->fmt.pix.field = V4L2_FIELD_INTERLACED;
     if( f->fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV ){
        f->fmt.pix.bytesperline = f->fmt.pix.width << 1;
        f->fmt.pix.colorspace   = V4L2_COLORSPACE_SMPTE170M;
     }else{
        f->fmt.pix.bytesperline = f->fmt.pix.width * 3;
        f->fmt.pix.colorspace   = V4L2_COLORSPACE_SRGB;
     }
     f->fmt.pix.sizeimage = f->fmt.pix.bytesperline * f->fmt.pix.height;

    return 0;
}

int vcdev_s_fmt_vid_cap( struct file * file, void * priv,
                                 struct v4l2_format * f)
{
    struct vc_device * dev;
    struct vb2_queue *q;
    int ret;

    PRINT_DEBUG( "IOCTL set_fmt\n" );

    dev = ( struct vc_device * ) video_drvdata( file );
    q = &dev->vb_out_vidq;

    if( vb2_is_busy(q) ){
        return -EBUSY;
    }

    return 0;

    ret = vcdev_try_fmt_vid_cap( file, priv, f);
    if ( ret < 0 )
         return ret;

    dev->output_format = f->fmt.pix; 
    return 0;
}

int vcdev_enum_frameintervals(struct file *file, void *priv,
                                     struct v4l2_frmivalenum *fival)
{
    struct vc_device * dev;
    struct v4l2_frmival_stepwise * frm_step;
    #ifdef DEBUG
        char str[5];
        memcpy((void *)str,(void *)&fival->pixel_format, 4);
        str[4] = 0;
        PRINT_DEBUG("IOCTL enum_frameintervals(%d), fmt(%s)\n",fival->index, str);
    #endif

    

    dev = ( struct vc_device * ) video_drvdata( file );

    if( fival->index > 0 ){
        PRINT_DEBUG("Index out of range\n");
        return -EINVAL;
    }

    if( !check_supported_pixfmt( dev, fival->pixel_format) ){
        PRINT_DEBUG("Unsupported pixfmt\n");
        return -EINVAL;
    }

    if( !dev->conv_res_on ){
        if( (fival->width != dev->input_format.width) 
            || (fival->height != dev->input_format.height) ){
            PRINT_DEBUG("Unsupported resolution\n");
            return -EINVAL;
        }
    }

    fival->type = V4L2_FRMIVAL_TYPE_STEPWISE;
    frm_step = &fival->stepwise;
    frm_step->min.numerator    = 1001;
    frm_step->min.denominator  = 60000;
    frm_step->max.numerator    = 1;
    frm_step->max.denominator  = 1;
    frm_step->step.numerator   = 1;
    frm_step->step.denominator = 1;

    return 0;
}

int vcdev_g_parm(struct file *file, void * priv,
                struct v4l2_streamparm * sp)
{
    struct vc_device * dev;
    struct v4l2_captureparm * cp;

    PRINT_DEBUG("IOCTL g_parm\n");

    if( sp->type != V4L2_BUF_TYPE_VIDEO_CAPTURE ){
        return -EINVAL;
    }
   
    cp = &sp->parm.capture;
    dev = ( struct vc_device * ) video_drvdata( file );

    memset( cp, 0x00, sizeof( struct v4l2_captureparm ) );
    cp->capability   = V4L2_CAP_TIMEPERFRAME;
    cp->timeperframe = dev->output_fps;
    cp->extendedmode = 0;
    cp->readbuffers  = 1;

    return 0;
}

int vcdev_s_parm(struct file *file, void * priv,
                struct v4l2_streamparm * sp)
{
    struct vc_device * dev;
    struct v4l2_captureparm * cp;

    PRINT_DEBUG("IOCTL s_parm\n");

    if( sp->type != V4L2_BUF_TYPE_VIDEO_CAPTURE ){
        return -EINVAL;
    }

    cp = &sp->parm.capture;
    dev = ( struct vc_device * ) video_drvdata( file );

    if( !cp->timeperframe.numerator || !cp->timeperframe.denominator ){
        cp->timeperframe = dev->output_fps;
    }else{
        dev->output_fps = cp->timeperframe;
    }
    cp->extendedmode = 0;
    cp->readbuffers  = 1;

    PRINT_DEBUG( "FPS set to %d/%d\n", cp->timeperframe.numerator, 
        cp->timeperframe.denominator );
    return 0;
}

int vcdev_enum_framesizes(struct file *filp, void *priv,
                                     struct v4l2_frmsizeenum * fsize)
{
    struct vc_device * dev;
    struct v4l2_frmsize_discrete * size_discrete;

    PRINT_DEBUG("IOCTL enum_framesizes\n");

    dev = ( struct vc_device * ) video_drvdata( filp );

    if( !check_supported_pixfmt( dev, fsize->pixel_format) ){
        return -EINVAL;
    }

    if( !dev->conv_res_on ){

        if( fsize->index > 0 ){
            return -EINVAL;
        }

        fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        size_discrete = &fsize->discrete;
        size_discrete->width  = dev->input_format.width;
        size_discrete->height = dev->input_format.height; 

    }else{

        if( fsize->index > 0 ){
            return -EINVAL;
        }

        fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
    }

    return 0;
}