#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../src/vcmod_api.h"

const char * short_options = "hcs:p:d";

const struct option long_options[] = {
	{ "help",       0, NULL, 'h' },
	{ "create",     0, NULL, 'c' },
	{ "size",       1, NULL, 's' },
	{ "pixfmt",     1, NULL, 'p' },
	{ "device",     1, NULL, 'd' },
	{ NULL,         0, NULL,  0  }
};

const char * help = " -h --help                    print this informations\n"
                    " -c --create                  create new device\n"
                    " -s --size    WIDTHxHEIGHT    specify resolution\n"
                    " -p --pixfmt  pix_fmt         pixel format (rgb24,yuv)\n"
                    " -d --device  /dev/*          control device node\n";

enum ACTION { ACTION_NONE, ACTION_CREATE, ACTION_DESTROY, ACTION_MODIFY };

struct vcmod_device_spec device_template = {
	.width  = 640,
	.height = 480,
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
	}else if( !strncmp(pixfmt_str,"yuv",3) ){
		return VCMOD_PIXFMT_YUV;
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

	res = ioctl(fd, VCMOD_IOCTL_CREATE_DEVICE, dev);

	close( ctrl_location );
	return res;
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
	}

	return ret;
}
