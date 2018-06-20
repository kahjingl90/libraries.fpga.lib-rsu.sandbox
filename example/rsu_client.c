// SPDX-License-Identifier: BSD-2-Clause

/* Intel Copyright 2018 */

#include <getopt.h>
#include <librsu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * enum rsu_clinet_command_code - supporting RSU client commands
 * COMMAND_
 */
enum rsu_clinet_command_code {
	COMMAND_ADD_IMAGE = 0,
	COMMAND_VERIFY_IMAGE,
	COMMAND_COPY_TO_FILE
};

static const struct option opts[] = {
	{"count", no_argument, NULL, 'c'},
	{"log", no_argument, NULL, 'g'},
	{"help", no_argument, NULL, 'h'},
	{"list", required_argument, NULL, 'l'},
	{"size", required_argument, NULL, 'z'},
	{"priority", required_argument, NULL, 'p'},
	{"enable", required_argument, NULL, 'E'},
	{"disable", required_argument, NULL, 'D'},
	{"add", required_argument, NULL, 'a'},
	{"slot", required_argument, NULL, 's'},
	{"erase", required_argument, NULL, 'e'},
	{"verify", required_argument, NULL, 'v'},
	{"copy", required_argument, NULL, 'f'},
	{"request", required_argument, NULL, 'r'},
	{NULL, 0, NULL, 0}
};

/*
 * rsu_client_usage() - show the usage of client application
 *
 * This function doesn't have return.
 */
static void rsu_client_usage(void)
{
	printf("--- RSU app usage ---\n");
	printf("%-32s  %s", "-c|--count", "get the number of slots\n");
	printf("%-32s  %s", "-l|--list slot_num",
	       "list the attribute info from the selected slot\n");
	printf("%-32s  %s", "-z|--size slot_num",
	       "get the slot size in bytes\n");
	printf("%-32s  %s", "-p|--priority slot_num",
	       "get the priority of the selected slot\n");
	printf("%-32s  %s", "-E|--enable slot_num",
	       "set the selected slot as the highest priority\n");
	printf("%-32s  %s", "-D|--disable slot_num",
	       "disable selected slot but to not erase it\n");
	printf("%-32s  %s", "-a|--add file_name [-s|--slot] slot_num",
	       "add a new app image to the selected slot, the default slot is 0 if user doesn't specify\n");
	printf("%-32s  %s", "-e|--erase slot_num",
	       "erase app image from the selected slot\n");
	printf("%-32s  %s", "-v|--verify file_name [-s|--slot] slot_num",
	       "verify app image on the selected slot, the default slot is 0 if user doesn't specify\n");
	printf("%-32s  %s", "-f|--copy file_name -s|--slot slot_num",
	       "read the data in a selected slot then write to a file\n");
	printf("%-32s  %s", "-r|--request slot_num",
	       "request the selected slot to be loaded after the next reboot\n");
	printf("%-32s  %s", "-g|--log", "print the status log\n");
	printf("%-32s  %s", "-h|--help", "show usage message\n");
}

/*
 * rsu_client_slot_count() - get the number of predefined slot
 *
 * Return: number of slots
 */
static int rsu_client_get_slot_count(void)
{
	return rsu_slot_count();
}

/*
 * rsu_client_copy_status_log() - copy status log
 *
 * This function copies the SDM status log info to struct rsu_status_info
 *
 * Return: 0 on success, -1 on error
 */
static int rsu_client_copy_status_log(void)
{
	struct rsu_status_info *info;
	int rtn = -1;

	info = (struct rsu_status_info *)malloc(sizeof(*info));
	if (!info) {
		printf("%s: fail to allocate\n", __func__);
		return rtn;
	}

	if (!rsu_status_log(info)) {
		rtn = 0;
		printf("      VERSION: 0x%08X\n", (int)info->version);
		printf("        STATE: 0x%08X\n", (int)info->state);
		printf("CURRENT IMAGE: 0x%016llX\n", info->current_image);
		printf("   FAIL IMAGE: 0x%016llX\n", info->fail_image);
		printf("    ERROR LOC: 0x%08X\n", (int)info->error_location);
		printf("ERROR DETAILS: 0x%08X\n", (int)info->error_details);
	}

	free(info);

	return rtn;
}

/*
 * rsu_client_request_slot_be_loaded() - Request the selected slot be loaded
 * slot_num: the slected slot
 *
 * This function requests that the selected slot be loaded after the next
 * reboot.
 *
 * Return: 0 on success, or negative value on error
 */
static int rsu_client_request_slot_be_loaded(int slot_num)
{
	return rsu_slot_load_after_reboot(slot_num);
}

/*
 * rsu_client_list_slot_attribute() - list the attribute of a slot
 * slot_num: the slected slot
 *
 * This function lists the attributes of a selected slot. The attributes
 * are image name, offset, size and priority level.
 *
 * Return: 0 on success, or -1 on error
 */
static int rsu_client_list_slot_attribute(int slot_num)
{
	struct rsu_slot_info *info;
	int rtn = -1;

	info = (struct rsu_slot_info *)malloc(sizeof(*info));
	if (!info) {
		printf("%s: fail to allocate\n", __func__);
		return rtn;
	}

	if (!rsu_slot_get_info(slot_num, info)) {
		rtn = 0;
		printf("      NAME: %s\n", info->name);
		printf("    OFFSET: 0x%016llX\n", info->offset);
		printf("      SIZE: 0x%08X\n", info->size);

		if (info->priority)
			printf("  PRIORITY: %i\n", info->priority);
		else
			printf("  PRIORITY: [disabled]\n");
	}

	free(info);

	return rtn;
}

/*
 * rsu_client_get_slot_size() - get the size for a selected slot
 * slot_num: a selected slot
 *
 * Return: size of the selected slot on success, or negative value on error
 */
static int rsu_client_get_slot_size(int slot_num)
{
	return rsu_slot_size(slot_num);
}

/*
 * rsu_client_get_priority() - get the priority for a selected slot
 * slot_num: a selected slot
 *
 * Return: 0 on success, or negative value on error
 */
static int rsu_client_get_priority(int slot_num)
{
	return rsu_slot_priority(slot_num);
}

/*
 * rsu_add_app_image() - add a new application image
 * image_name: name of the application image
 * slot_name: the selected slot
 *
 * Return: 0 on success, or negative value on error
 */
static int rsu_client_add_app_image(char *image_name, int slot_num)
{
	return rsu_slot_program_file(slot_num, image_name);
}

/*
 * rsu_client_erase_image() - erase the application image from a selected slot
 * slot_num: the slot number
 *
 * Return: 0 on success, or negative value on error.
 */
static int rsu_client_erase_image(int slot_num)
{
	return rsu_slot_erase(slot_num);
}

/*
 * rsu_client_verify_data() - verify the data in selected slot compared to file
 * file_name: file name
 * slot_num: the selected slot
 *
 * Return: 0 on success, or negativer on error.
 */
static int rsu_client_verify_data(char *file_name, int slot_num)
{
	return rsu_slot_verify_file(slot_num, file_name);
}

/*
 * rsu_client_copy_to_file() - read the data from a slot then write to file
 * file_name: number of file which store the data
 * slot_num: the selected slot
 *
 * Return: 0 on success, or negative on error
 */
static int rsu_client_copy_to_file(char *file_name, int slot_num)
{
	return rsu_slot_copy_to_file(slot_num, file_name);
}

static void error_handle(void)
{
	printf(" operation is failure\n");
	librsu_exit();
	exit(1);
}

int main(int argc, char *argv[])
{
	int c;
	int index = 0;
	int slot_num_default = 0;
	int slot_num;
	enum rsu_clinet_command_code command;
	char *filename;
	int slot_info;
	int ret;

	if (argc == 1) {
		rsu_client_usage();
		exit(1);
	}

	ret = librsu_init("");
	if (ret) {
		printf("librsu_init return %d\n", ret);
		return ret;
	}

	while ((c = getopt_long(argc, argv,
				"cghl:z:p:t:a:s:e:v:f:r:E:D:",
				opts, &index)) != -1) {
		switch (c) {
		case 'c':
			slot_info = rsu_client_get_slot_count();
			printf("number of slots is %d\n", slot_info);
			break;
		case 'l':
			if (rsu_client_list_slot_attribute(atoi(optarg)))
				error_handle();
			break;
		case 'z':
			slot_info = rsu_client_get_slot_size(atoi(optarg));
			printf("size of the selected slot is %d\n", slot_info);
			if (slot_info == -1)
				error_handle();
			break;
		case 'p':
			slot_info = rsu_client_get_priority(atoi(optarg));
			printf("priority of the selected slot is %d\n",
			       slot_info);
			if (slot_info == -1)
				error_handle();
			break;
		case 'E':
			if (rsu_slot_enable(atoi(optarg)))
				error_handle();
			break;
		case 'D':
			if (rsu_slot_disable(atoi(optarg)))
				error_handle();
			break;
		case 'a':
			filename = optarg;
			/*
			 * if user doesn't specify the slot, then write image
			 * to the default slot (slot = 0)
			 */
			if (argc == 3) {
				if (rsu_client_add_app_image
				    (filename, slot_num_default))
					error_handle();
			} else {
				command = COMMAND_ADD_IMAGE;
			}
			break;
		case 'r':
			if (rsu_client_request_slot_be_loaded(atoi(optarg)))
				error_handle();
			break;
		case 's':
			slot_num = atoi(optarg);
			switch (command) {
			case COMMAND_ADD_IMAGE:
				ret =
				    rsu_client_add_app_image(filename,
							     slot_num);
				break;
			case COMMAND_VERIFY_IMAGE:
				ret =
				    rsu_client_verify_data(filename, slot_num);
				break;
			case COMMAND_COPY_TO_FILE:
				ret = rsu_client_copy_to_file(filename,
							      slot_num);
				break;
			default:
				break;
			}

			if (ret)
				error_handle();
			break;
		case 'e':
			if (rsu_client_erase_image(atoi(optarg)))
				error_handle();
			break;
		case 'v':
			filename = optarg;
			/* if user doesn't specify the slot */
			if (argc == 3) {
				if (rsu_client_verify_data
				    (filename, slot_num_default))
					error_handle();
			} else {
				command = COMMAND_VERIFY_IMAGE;
			}
			break;
		case 'f':
			if (argc == 3) {
				rsu_client_usage();
				exit(1);
			}

			filename = optarg;
			command = COMMAND_COPY_TO_FILE;
			break;
		case 'g':
			if (rsu_client_copy_status_log())
				error_handle();
			break;
		case 'h':
			rsu_client_usage();
			librsu_exit();
			exit(0);

		default:
			rsu_client_usage();
			librsu_exit();
			exit(1);
		}
	}

	printf("Operation completed\n");

	librsu_exit();
	return 0;
}