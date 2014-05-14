#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../src/vcmod_api.h"

const char * short_options = "hcm:r:ls:p:d:";

const struct option long_options[] = {
	{ "help",       0, NULL, 'h' },
	{ "create",     0, NULL, 'c' },
	{ "modify",     1, NULL, 'm' },
	{ "list"  ,     0, NULL, 'l' },
	{ "size",       1, NULL, 's' },
	{ "pixfmt",     1, NULL, 'p' },
	{ "device",     1, NULL, 'd' },
	{ "remove",     1, NULL, 'r' },
	{ NULL,         0, NULL,  0  }
};

const char * help = " -h --help                    print this informations\n"
                    " -c --create                  create new device\n"
                    " -m --modify  idx             modify device\n"
                    " -r --remove  idx             remove device\n"
                    " -l --list                    list devices\n"
                    " -s --size    WIDTHxHEIGHT    specify resolution\n"
                    " -p --pixfmt  pix_fmt         pixel format (rgb24,yuv)\n"
                    " -d --device  /dev/*          control device node\n";

enum ACTION { ACTION_NONE, ACTION_CREATE, ACTION_DESTROY, ACTION_MODIFY };

struct vcmod_device_spec device_template = {
	.width  = 640,
	.height = 480,
	.pix_fmt = VCMOD_PIXFMT_RGB24,
	.video_node = "",
	.fb_node = "",
};

char ctrl_location[128] = "/dev/vcdev";

void print_help()
{
	printf("%s",help);
}

int parse_resolution( char * res_str , struct vcmod_device_spec * dev )
{
	char * tmp;
	tmp = strtok(res_str,"x:,");
	if(tmp == NULL){
		return -1;
	}
	dev->width = atoi( tmp );
	tmp = strtok(NULL,"x:,");
	if( tmp == NULL ){
		return -1;
	}
	dev->height = atoi( tmp );
	return 0;
}

int determine_pixfmt( char * pixfmt_str )
{
	if( !strncmp( pixfmt_str, "rgb24", 5) ){
		return VCMOD_PIXFMT_RGB24;
	}else if( !strncmp(pixfmt_str,"yuyv",3) ){
		return VCMOD_PIXFMT_YUYV;
	}else {
		return -1;
	}
}

int create_device( struct vcmod_device_spec * dev )
{
	int fd;
	int res = 0;

	fd = open( ctrl_location, O_RDWR );
	if( fd == -1 ){
		fprintf(stderr,"Failed to open %s device\n",ctrl_location);
		return;
	}

	if(!dev->width || !dev->height){
		dev->width = device_template.width;
		dev->height = device_template.height;
	}

	if(!dev->pix_fmt){
		dev->pix_fmt = device_template.pix_fmt;
	}

	res = ioctl(fd, VCMOD_IOCTL_CREATE_DEVICE, dev);

	close( ctrl_location );
	return res;
}

int remove_device( struct vcmod_device_spec * dev )
{
	int fd;
	int res = 0;

	fd = open( ctrl_location, O_RDWR );
	if( fd == -1 ){
		fprintf(stderr, "Failed to open %s device\n", ctrl_location );
		return -1;
	}

	if( ioctl( fd, VCMOD_IOCTL_DESTROY_DEVICE, dev) ){
		fprintf(stderr, "Can't remove device with index %d\n", dev->idx + 1);
		return -1;
	}
	close(fd);
	printf("Device removed\n");
	return 0;
}

int modify_device( struct vcmod_device_spec * dev )
{
	int fd;
	int res = 0;

	struct vcmod_device_spec orig_dev;
	orig_dev.idx = dev->idx;

	fd = open( ctrl_location, O_RDWR );
	if( fd == -1 ){
		fprintf(stderr,"Failed to open %s device\n",ctrl_location);
		return -1;
	}

	if( ioctl(fd, VCMOD_IOCTL_GET_DEVICE, &orig_dev)){
		fprintf(stderr, "No device with index %d\n", orig_dev.idx + 1);
		return -1;
	}

	if(!dev->width || !dev->height){
		dev->width = orig_dev.width;
		dev->height = orig_dev.height;
	}

	if(!dev->pix_fmt){
		dev->pix_fmt = orig_dev.pix_fmt;
	}

	res = ioctl( fd, VCMOD_IOCTL_MODIFY_SETTING, dev );
	printf("Setting modified\n");

	close(fd);

	return res;
}

int list_devices()
{
	int fd;
	struct vcmod_device_spec dev;
	int idx;

	fd = open( ctrl_location, O_RDWR );
	if( fd == -1 ){
		fprintf(stderr,"Failed to open %s device\n",ctrl_location);
		return -1;
	}

	printf("Virtual V4L2 devices:\n");
	dev.idx = 0;
	while( !ioctl(fd, VCMOD_IOCTL_GET_DEVICE, &dev) ){
		dev.idx++;
		printf("%d. %s(%d,%d,%s) -> %s\n",
			dev.idx, dev.fb_node, dev.width, dev.height, 
			dev.pix_fmt == VCMOD_PIXFMT_RGB24 ? "rgb24":"yuyv",
			dev.video_node);
	}
	close(fd);
	return 0;
}

int main( int argc, char * argv[])
{
	char * program_name;
	int next_option;
	enum ACTION current_action = ACTION_NONE;
	struct vcmod_device_spec dev;
    int ret = 0;
    int tmp;

	program_name = argv[0];
	memset(&dev, 0x00, sizeof(struct vcmod_device_spec));

	//Process cmd line options
	do{
		next_option = getopt_long( argc, argv, short_options, long_options, NULL );
		switch( next_option ){
			case 'h':
				print_help();
				exit(0);
			case 'c':
				current_action = ACTION_CREATE;
				printf("A new device will be created\n");
				break;
			case 'm':
				current_action = ACTION_MODIFY;
				dev.idx = atoi(optarg) - 1;
				break;
			case 'r':
				current_action = ACTION_DESTROY;
				dev.idx = atoi(optarg) -1;
				break;
			case 'l':
				list_devices();
				break;
			case 's':
				if(parse_resolution(optarg,&dev)){
					printf("Failed to parse resolution");
					exit( -1 );
				}
				printf("Setting resolution to %dx%d\n",dev.width,dev.height);
				break;
			case 'p':
				tmp = determine_pixfmt( optarg );
				if ( tmp < 0 ){
					fprintf(stderr, "Unknown pixel format %s\n", optarg );
					exit(-1);
				}
				dev.pix_fmt = (char)tmp;
				printf("Setting pixel format to %s\n", optarg );
				break;
			case 'd':
				printf("Using device %s\n", optarg );
				strncpy( ctrl_location, optarg, sizeof(ctrl_location) - 1 );
				break;
		}
	}while( next_option != -1 );

	switch(current_action){
		case ACTION_CREATE:
			ret = create_device( &dev );
			break;
		case ACTION_DESTROY:
			ret = remove_device( &dev );
			break;
		case ACTION_MODIFY:
			ret = modify_device( &dev );
			break;
	}

	return ret;
}
