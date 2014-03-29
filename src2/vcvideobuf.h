#ifndef VCVIDEOBUF_H
#define VCVIDEOBUF_H

#include <media/videobuf2-core.h>
#include <media/videobuf2-vmalloc.h>
#include "vcdevice.h"

int vc_out_videobuf2_setup( struct vc_device * dev );

int vc_out_queue_setup( struct vb2_queue * vq,
                         const struct v4l2_format * fmt,
                         unsigned int *nbuffers, unsigned int *nplanes,
                         unsigned int sizes[], void * alloc_ctxs[]);

int vc_out_buffer_prepare( struct vb2_buffer * vb );

void vc_out_buffer_queue( struct vb2_buffer * vb );

int vc_start_streaming( struct vb2_queue * q, unsigned int count );

int vc_stop_streaming( struct vb2_queue * q );

void vc_outbuf_lock( struct vb2_queue * vq );

void vc_outbuf_unlock( struct vb2_queue * vq );

struct vc_out_buffer{
	struct vb2_buffer vb;
};

#endif