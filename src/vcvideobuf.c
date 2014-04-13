#include "vcvideobuf.h"
#include "debug.h"
#include <linux/spinlock.h>

static const struct vb2_ops vc_vb2_ops = {
    .queue_setup     = vc_out_queue_setup,
    .buf_prepare     = vc_out_buffer_prepare,
    .buf_queue       = vc_out_buffer_queue,
    .start_streaming = vc_start_streaming,
    .stop_streaming  = vc_stop_streaming,
    .wait_prepare    = vc_outbuf_unlock,
    .wait_finish     = vc_outbuf_lock,
};

int vc_out_videobuf2_setup( struct vc_device * dev )
{
	int ret;
	struct vb2_queue * q;
	
	PRINT_DEBUG( "setting up videobuf2 queue\n" );

	q = &dev->vb_out_vidq;
    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
    q->drv_priv = dev;
    q->buf_struct_size = sizeof(struct vc_out_buffer);
    q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    q->ops = &vc_vb2_ops;
    q->mem_ops = &vb2_vmalloc_memops;

    ret = vb2_queue_init(q);
	return ret;
}

int vc_out_queue_setup( struct vb2_queue * vq,
                         const struct v4l2_format * fmt,
                         unsigned int *nbuffers, unsigned int *nplanes,
                         unsigned int sizes[], void * alloc_ctxs[])
{
    unsigned long size;    
    //struct virtualcam_device * dev = vb2_get_drv_priv(vq);
    PRINT_DEBUG( "queue_setup\n" );

    if( fmt )
        size = fmt->fmt.pix.sizeimage;
    else
        size = 640*480*3;
   
    if( 0 == *nbuffers )
        *nbuffers = 32;
 
    *nplanes = 1;
    
    sizes[0] = size;
    PRINT_DEBUG( "queue_setup completed\n" ); 
    return 0;
}

int vc_out_buffer_prepare( struct vb2_buffer * vb )
{
    unsigned long size;
    void * data;
    char color;
    int i;
    PRINT_DEBUG( "buffer_prepare\n");
    
    size = 480*640*3;
    if( vb2_plane_size(vb,0) < size ){
        PRINT_ERROR( KERN_ERR "data will not fit into buffer\n" );
        return -EINVAL;
    }

    vb2_set_plane_payload(vb,0,size);
    data = (void*)vb2_plane_vaddr(vb,0);
    if(!data){
      PRINT_DEBUG( "Strange thing happened in the buffer prepare\n");
      goto error;
    }
    color = 0xff;
    for( i = 0; i < 240; i++){
    	memset(data,color,640*2*3); //set to white color
    	color--;
    	data+=640*2*3;
    }
    
    error:
    return 0;
}

void vc_out_buffer_queue( struct vb2_buffer * vb )
{
    struct vc_device * dev;
    struct vc_out_buffer * buf;
    struct vc_out_queue * q;
    unsigned long flags = 0;

    PRINT_DEBUG( "buffer_queue\n" );

    dev = vb2_get_drv_priv( vb->vb2_queue );
    buf = container_of( vb, struct vc_out_buffer, vb );
    q = &dev->vc_out_vidq;
    buf->filled = 0;

    spin_lock_irqsave( &dev->out_q_slock, flags );
    list_add_tail( &buf->list, &q->active );
    spin_unlock_irqrestore( &dev->out_q_slock, flags );
}

int vc_start_streaming( struct vb2_queue * q, unsigned int count )
{
    PRINT_DEBUG( "start streaming vb called\n");
    //TODO
    return 0;
}

int vc_stop_streaming( struct vb2_queue * q )
{
    PRINT_DEBUG( "stop streaming vb called\n");
    //TODO
    return 0;
}

void vc_outbuf_lock( struct vb2_queue * vq )
{
    struct vc_device * dev = vb2_get_drv_priv(vq);
    mutex_lock( &dev->vc_mutex );
}

void vc_outbuf_unlock( struct vb2_queue * vq )
{
    struct vc_device * dev = vb2_get_drv_priv(vq);
    mutex_unlock( &dev->vc_mutex );
}