/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014 Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "hciattach.h"

#ifndef FIRMWARE_DIR
#define FIRMWARE_DIR "/etc/firmware"
#endif

#define FW_EXT ".hcd"

#define BCM43XX_CLOCK_48 1
#define BCM43XX_CLOCK_24 2

#define CMD_SUCCESS 0x00

#define CC_MIN_SIZE 7

#define CONFIG_NBS 0
#define CONFIG_WBS 1

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

static int bcm43xx_read_local_name(int fd, char *name, size_t size)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x14, 0x0C, 0x00 };
	unsigned char *resp;
	unsigned int name_len;

	resp = malloc(size + CC_MIN_SIZE);
	if (!resp)
		return -1;

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write read local name command\n");
		goto fail;
	}

	if (read_hci_event(fd, resp, size) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to read local name, invalid HCI event\n");
		goto fail;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to read local name, command failure\n");
		goto fail;
	}

	name_len = resp[2] - 1;

	strncpy(name, (char *) &resp[7], MIN(name_len, size));
	name[size - 1] = 0;

	free(resp);
	return 0;

fail:
	free(resp);
	return -1;
}

static int bcm43xx_reset(int fd)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x03, 0x0C, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write reset command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to reset chip, invalid HCI event\n");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to reset chip, command failure\n");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_bdaddr(int fd, bdaddr_t *bdaddr)
{
	unsigned char cmd[] =
		{ HCI_COMMAND_PKT, 0x01, 0xfc, 0x06, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00 };
	unsigned char resp[CC_MIN_SIZE];
	int i;

	printf("Set BDADDR UART:");
	for (i = 5; i >= 0; i--)
		printf(" %02x", bdaddr->b[i]);
	printf("\n");

	memcpy(&cmd[4], bdaddr, 6);

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write set bdaddr command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to set bdaddr, invalid HCI event\n");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to set bdaddr, command failure\n");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_sco_config(int fd, uint32_t speech_cfg)
{
	// default config (NBS)
	unsigned char cmd_i2spcm_interface_param[] =
		{ HCI_COMMAND_PKT, 0x6d, 0xfc, 0x04, 0x00, 0x00, 0x00,
			0x00 };
	unsigned char cmd_sco_pcm_int_param[] =
		{ HCI_COMMAND_PKT, 0x1c, 0xfc, 0x05, 0x00, 0x00, 0x00,
			0x00, 0x00 };
	unsigned char cmd_sco_pcm_data_format[] =
		{ HCI_COMMAND_PKT, 0x1e, 0xfc, 0x05, 0x00, 0x00, 0x03,
			0x03, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	if(speech_cfg == CONFIG_WBS) {
		printf("Set SCO config to WBS\n");
		cmd_i2spcm_interface_param[4] = (uint8_t) (0x01);
		cmd_i2spcm_interface_param[5] = (uint8_t) (0x01);
		cmd_sco_pcm_int_param[5] = (uint8_t) (0x01);
	} else {
		printf("Set SCO config to default (NBS)\n");
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd_i2spcm_interface_param,
	    sizeof(cmd_i2spcm_interface_param)) != sizeof(cmd_i2spcm_interface_param)) {
		fprintf(stderr, "Failed to write I2SPCM_INTERFACE_PARAM command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to write I2SPCM_INTERFACE_PARAM, invalid HCI event\n");
		return -1;
	}

	if (resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to write I2SPCM_INTERFACE_PARAM, command failure\n");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd_sco_pcm_int_param,
            sizeof(cmd_sco_pcm_int_param)) != sizeof(cmd_sco_pcm_int_param)) {
		fprintf(stderr, "Failed to write SCO_PCM_INT_PARAM command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to write SCO_PCM_INT_PARAM, invalid HCI event\n");
		return -1;
	}

	if (resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to write SCO_PCM_INT_PARAM, command failure\n");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd_sco_pcm_data_format,
            sizeof(cmd_sco_pcm_data_format)) != sizeof(cmd_sco_pcm_data_format)) {
		fprintf(stderr, "Failed to write WRITE_PCM_DATA_FORMAT_PARAM command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to write WRITE_PCM_DATA_FORMAT_PARAM, invalid HCI event\n");
		return -1;
	}

	if (resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to write WRITE_PCM_DATA_FORMAT_PARAM, command failure\n");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_clock(int fd, unsigned char clock)
{
        unsigned char cmd[] = { HCI_COMMAND_PKT, 0x45, 0xfc, 0x01, 0x00 };
        unsigned char resp[CC_MIN_SIZE];

        printf("Set Controller clock (%d)\n", clock);

        cmd[4] = clock;

        tcflush(fd, TCIOFLUSH);

        if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
                fprintf(stderr, "Failed to write update clock command\n");
                return -1;
        }

        if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
                fprintf(stderr, "Failed to update clock, invalid HCI event\n");
                return -1;
        }

        if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
                fprintf(stderr, "Failed to update clock, command failure\n");
                return -1;
        }

        return 0;
}

static int bcm43xx_set_speed(int fd, struct termios *ti, uint32_t speed)
{
	unsigned char cmd[] =
		{ HCI_COMMAND_PKT, 0x18, 0xfc, 0x06, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	if (speed > 3000000 && bcm43xx_set_clock(fd, BCM43XX_CLOCK_48))
		return -1;

	printf("Set Controller UART speed to %d bit/s\n", speed);

	cmd[6] = (uint8_t) (speed);
	cmd[7] = (uint8_t) (speed >> 8);
	cmd[8] = (uint8_t) (speed >> 16);
	cmd[9] = (uint8_t) (speed >> 24);

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write update baudrate command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to update baudrate, invalid HCI event\n");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to update baudrate, command failure\n");
		return -1;
	}

	if (set_speed(fd, ti, speed) < 0) {
		perror("Can't set host baud rate");
		return -1;
	}

	return 0;
}

static int bcm43xx_load_firmware(int fd, const char *fw)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x2e, 0xfc, 0x00 };
	struct timespec tm_mode = { 0, 50000000 };
	struct timespec tm_ready = { 0, 2000000 };
	unsigned char resp[CC_MIN_SIZE];
	unsigned char tx_buf[1024];
	int len, fd_fw, n;

	printf("Flash firmware %s\n", fw);

	fd_fw = open(fw, O_RDONLY);
	if (fd_fw < 0) {
		fprintf(stderr, "Unable to open firmware (%s)\n", fw);
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write download mode command\n");
		goto fail;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to load firmware, invalid HCI event\n");
		goto fail;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to load firmware, command failure\n");
		goto fail;
	}

	/* Wait 50ms to let the firmware placed in download mode */
	nanosleep(&tm_mode, NULL);

	tcflush(fd, TCIOFLUSH);

	while ((n = read(fd_fw, &tx_buf[1], 3))) {
		if (n < 0) {
			fprintf(stderr, "Failed to read firmware\n");
			goto fail;
		}

		tx_buf[0] = HCI_COMMAND_PKT;

		len = tx_buf[3];

		if (read(fd_fw, &tx_buf[4], len) < 0) {
			fprintf(stderr, "Failed to read firmware\n");
			goto fail;
		}

		if (write(fd, tx_buf, len + 4) != (len + 4)) {
			fprintf(stderr, "Failed to write firmware\n");
			goto fail;
		}

		read_hci_event(fd, resp, sizeof(resp));
		tcflush(fd, TCIOFLUSH);
	}

	/* Wait for firmware ready */
	nanosleep(&tm_ready, NULL);

	close(fd_fw);
	return 0;

fail:
	close(fd_fw);
	return -1;
}

static int bcm43xx_locate_patch(const char *dir_name,
		const char *chip_name, char *location)
{
	DIR *dir;
	int ret = -1;

	dir = opendir(dir_name);
	if (!dir) {
		fprintf(stderr, "Cannot open directory '%s': %s\n",
				dir_name, strerror(errno));
		return -1;
	}

	/* Recursively look for a BCM43XX*.hcd */
	while (1) {
		struct dirent *entry = readdir(dir);
		if (!entry)
			break;

		if (entry->d_type & DT_DIR) {
			char path[PATH_MAX];

			if (!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, "."))
				continue;

			snprintf(path, PATH_MAX, "%s/%s", dir_name, entry->d_name);

			ret = bcm43xx_locate_patch(path, chip_name, location);
			if (!ret)
				break;
		} else if (!strncmp(chip_name, entry->d_name, strlen(chip_name))) {
			unsigned int name_len = strlen(entry->d_name);
			size_t curs_ext = name_len - sizeof(FW_EXT) + 1;

			if (curs_ext > name_len)
				break;

			if (strncmp(FW_EXT, &entry->d_name[curs_ext], sizeof(FW_EXT)))
				break;

			/* found */
			snprintf(location, PATH_MAX, "%s/%s", dir_name, entry->d_name);
			ret = 0;
			break;
		}
	}

	closedir(dir);

	return ret;
}

static int bcm43xx_sleep_mode(int fd)
{
	unsigned char resp[CC_MIN_SIZE];
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x27, 0xfc, 0x0c,
				0x01, /* uart(1) */
				0x01, /* bt sleep timeout */
				0x01, /* host sleep timeout */
				0x01, /* bt wake, active high */
				0x01, /* host wake, active high */
				0x01, /* sleep during sco */
				0x01, /* request for sleep */
				0x00, /* fixed */
				0x00, /* fixed */
				0x00, /* fixed */
				0x00, /* fixed */
				0x00 }; /* fixed */

	printf("Configure sleep mode\n");

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write sllleep mode\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to write sleep mode,\
			invalid HCI event\n");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to write sleep mode,\
			command failure\n");
		return -1;
	}

	return 0;
}

static int bcm43xx_read_bdaddr(int fd, bdaddr_t *bdaddr)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x09, 0x10, 0x00 };
	unsigned char resp[CC_MIN_SIZE + 6];
	int i;

	printf("Read BD addr:");

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		fprintf(stderr, "Failed to write read bdaddr command\n");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		fprintf(stderr, "Failed to read bdaddr, invalid HCI event\n");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		fprintf(stderr, "Failed to read bdaddr, command failure\n");
		return -1;
	}

	memcpy(bdaddr, &resp[7], 6);

	for (i = 5; i >= 0; i--)
		printf(" %02x", bdaddr->b[i]);

	printf("\n");

	return 0;
}

const bdaddr_t bcm_def_addr = { {0x00, 0x00, 0x00, 0xb1, 0x30, 0x43} };
const bdaddr_t bcm_inv_addr = { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

static int bcm43xx_check_addr(const bdaddr_t *bdaddr) {
	if (!memcmp(bdaddr, &bcm_def_addr, 6)) {
		printf("warning: broadcom default bdaddr\n");
		return -1;
	}

	if (!memcmp(bdaddr, &bcm_inv_addr, 6)) {
		printf("warning: invalid bdaddr\n");
		return -1;
	}

	return 0;
}

int bcm43xx_init(int fd, int def_speed, int speed, struct termios *ti,
		const char *bdaddr, int pm)
{
	char chip_name[20];
	char fw_path[PATH_MAX];
	bdaddr_t init_bdaddr;
	bdaddr_t fw_bdaddr;
	bdaddr_t manual_bdaddr;

	printf("bcm43xx_init\n");

	if (bdaddr && (str2ba(bdaddr, &manual_bdaddr) < 0)) {
		fprintf(stderr, "Incorrect bdaddr\n");
		return -1;
	}

	bcm43xx_read_bdaddr(fd, &init_bdaddr);

	if (bcm43xx_reset(fd))
		return -1;

	if (bcm43xx_read_local_name(fd, chip_name, sizeof(chip_name)))
		return -1;

	if (bcm43xx_locate_patch(FIRMWARE_DIR, chip_name, fw_path)) {
		fprintf(stderr, "Patch not found, continue anyway\n");
	} else {
		if (bcm43xx_set_speed(fd, ti, speed))
			return -1;

		if (bcm43xx_load_firmware(fd, fw_path))
			return -1;

		/* Controller speed has been reset to def speed */
		if (set_speed(fd, ti, def_speed) < 0) {
			perror("Can't set host baud rate");
			return -1;
		}

		if (bcm43xx_reset(fd))
			return -1;
	}

	bcm43xx_read_bdaddr(fd, &fw_bdaddr);

	if (bdaddr)
		bcm43xx_set_bdaddr(fd, &manual_bdaddr);
	else if (bcm43xx_check_addr(&fw_bdaddr))
		bcm43xx_set_bdaddr(fd, &init_bdaddr);

	if (bcm43xx_set_sco_config(fd, CONFIG_NBS))
		return -1;

	if (pm)
		bcm43xx_sleep_mode(fd);

	if (bcm43xx_set_speed(fd, ti, speed))
		return -1;

	return 0;
}
