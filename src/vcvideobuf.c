#include "vcvideobuf.h"
#include "debug.h"
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

static const struct vb2_ops vc_vb2_ops = {
    .queue_setup     = vc_out_queue_setup,
    .buf_prepare     = vc_out_buffer_prepare,
    .buf_queue       = vc_out_buffer_queue,
    .start_streaming = vc_start_streaming,
    .stop_streaming  = vc_stop_streaming,
    .wait_prepare    = vc_outbuf_unlock,
    .wait_finish     = vc_outbuf_lock,
};

inline static int init_vc_in_buffer( struct vc_in_buffer * buf, size_t size )
{
    buf->data = vmalloc( size );
    if(!buf->data){
        PRINT_ERROR("Failed to allocate input buffer\n");
        return -ENOMEM;
    }
    buf->filled = 0;
    return 0;
}

inline static void destroy_vc_in_buffer( struct vc_in_buffer * buf )
{
    vfree( buf->data );
}

void swap_in_queue_buffers( struct vc_in_queue * q )
{
    struct vc_in_buffer * tmp;
    if( !q ){
        return;
    }
    //PRINT_DEBUG("Before swap R:%lX P:%lX",(long unsigned int)q->ready,(long unsigned int) q->pending);
    tmp = q->pending;
    q->pending = q->ready;
    q->ready = tmp;
    q->pending->filled = 0;
    //PRINT_DEBUG("After swap R:%lX P:%lX",(long unsigned int)q->ready,(long unsigned int) q->pending);
    //PRINT_DEBUG("Buffers swapped\n");
}

int vc_in_queue_setup( struct vc_in_queue * q, size_t size )
{
    int ret;
    int i;
    ret = 0;
    PRINT_DEBUG( "setting up input queue\n" );

    //Initialize buffers
    for( i = 0; i < 2; i++ ){
        ret = init_vc_in_buffer( &q->buffers[i], size );
        if( ret ){
            break;
        }
    }

    if( ret ){
        PRINT_ERROR("input queue alloc failure\n");
        for( ; i>0; i-- ){
            destroy_vc_in_buffer( &q->buffers[i-1] );
        }
        return ret;
    }

    //Initialize dummy buffer
    memset( &q->dummy, 0x00, sizeof(struct vc_in_buffer) );

    //Initialize pointers to buffers
    q->pending = &q->buffers[0];
    q->ready   = &q->buffers[1];

    return ret;
}

void vc_in_queue_destroy( struct vc_in_queue * q )
{
    int i;
    PRINT_DEBUG("Destroying input queue\n");
    for( i = 0; i < 2; i++ ){
        destroy_vc_in_buffer( &q->buffers[i] );
    }
}

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
    struct vc_device * dev;
    //struct virtualcam_device * dev = vb2_get_drv_priv(vq);
    PRINT_DEBUG( "queue_setup\n" );

    dev = vb2_get_drv_priv( vq );

    if( fmt )
        size = fmt->fmt.pix.sizeimage;
    else
        size = dev->output_format.sizeimage;

    PRINT_DEBUG("sizeimage set to %ld\n",size);
   
    if( 0 == *nbuffers )
        *nbuffers = 2;
 
    *nplanes = 1;
    
    sizes[0] = size;
    PRINT_DEBUG( "queue_setup completed\n" ); 
    return 0;
}

int vc_out_buffer_prepare( struct vb2_buffer * vb )
{
    unsigned long size;
    struct vc_device * dev;
    //void * data;
    //char color;
    //int i;

    //PRINT_DEBUG( "buffer_prepare\n");
    
    dev = vb2_get_drv_priv( vb->vb2_queue );

    size = dev->output_format.sizeimage;
    if( vb2_plane_size(vb,0) < size ){
        PRINT_ERROR( KERN_ERR "data will not fit into buffer\n" );
        return -EINVAL;
    }

    vb2_set_plane_payload(vb,0,size);
    /*data = (void*)vb2_plane_vaddr(vb,0);
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
    
    error:*/
    return 0;
}

void vc_out_buffer_queue( struct vb2_buffer * vb )
{
    struct vc_device * dev;
    struct vc_out_buffer * buf;
    struct vc_out_queue * q;
    unsigned long flags = 0;

    //PRINT_DEBUG( "buffer_queue\n" );

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
    struct vc_device * dev;
    PRINT_DEBUG( "start streaming vb called, count = %d\n", count);

    dev = q->drv_priv;

    //Try to start kernel thread
    dev->sub_thr_id = kthread_create( submitter_thread, dev, "vcam_submitter");
    if( !dev->sub_thr_id ){
        PRINT_ERROR("Failed to create kernel thread\n");
        return -ECANCELED;
    }

    wake_up_process(dev->sub_thr_id);

    return 0;
}

int vc_stop_streaming( struct vb2_queue * vb2_q )
{
    struct vc_device * dev;
    struct vc_out_buffer * buf;
    struct vc_out_queue  * q;
    unsigned long flags;
    PRINT_DEBUG( "stop streaming vb called\n");

    flags = 0;
    dev = vb2_q->drv_priv;
    q = &dev->vc_out_vidq;

    //Stop running threads
    if( dev->sub_thr_id ){
        kthread_stop( dev->sub_thr_id );
    }

    dev->sub_thr_id = NULL;
    //Empty buffer queue
    //spin_lock_irqsave( &dev->out_q_slock, flags );
    while ( !list_empty( &q->active ) ) {
            buf = list_entry( q->active.next, struct vc_out_buffer , list);
            list_del( &buf->list );
            vb2_buffer_done( &buf->vb, VB2_BUF_STATE_ERROR);
            PRINT_DEBUG("Throwing out buffer\n");
    }
    //spin_unlock_irqrestore( &dev->out_q_slock, flags );

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