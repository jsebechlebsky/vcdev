#include "vcioctl.h"
#include "debug.h"

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
    sprintf(inp->name,"vc_in %u", inp->index );
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
    if ( i>= 1 )
        return -EINVAL;
    return 0;
}