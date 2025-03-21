// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for I2C connected EETI EXC7200 multiple touch controller
 *
 * Copyright (C) 2017 Ahmet Inan <inan@distec.de>
 *
 * minimal implementation based on egalax_ts.c and egalax_i2c.c
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#define EXC7200_NUM_SLOTS		2
#define EXC7200_LEN_FRAME		10
#define EXC7200_ST1_EVENT		131
#define EXC7200_ST2_EVENT		130
#define EXC7200_MT1_EVENT		135
#define EXC7200_MT2_EVENT		134

#define EXC7200_TIMEOUT_MS		100

/* sensor resolution */
#define SENSOR_RES_X 4095
#define SENSOR_RES_Y 4095
/* Resolution diagonal */
#define SENSOR_AREA_LENGTH_LONGER          5792
/*((SIS_MAX_X^2) + (SIS_MAX_Y^2))^0.5*/
#define SENSOR_AREA_LENGTH_SHORT           5792
#define SENSOR_AREA_UNIT                   (5792 / 32)


static const struct i2c_device_id exc7200_id[];

u16 xa, ya, xb, yb, x, y, w, h, p;
int tType = 0; // touch type. 0 = single finger touch, 1 = two finger touch

struct eeti_dev_info {
	const char *name;
	int max_xy;
};

enum eeti_dev_id {
	EETI_EXC7200,
};

static struct eeti_dev_info exc7200_info[] = {
	[EETI_EXC7200] = {
		.name = "EETI EXC7200 Touch Screen",
		.max_xy = SZ_4K - 1,
	},
};

struct exc7200_data {
	struct i2c_client *client;
	const struct eeti_dev_info *info;
	struct input_dev *input;
	struct touchscreen_properties prop;
	struct gpio_desc *reset;
	struct timer_list timer;
	u8 buf[EXC7200_LEN_FRAME];
	struct completion wait_event;
	struct mutex query_lock;
};

static int exc7200_read_frame(struct exc7200_data *data, u8 *buf)
{
	struct i2c_client *client = data->client;
	int ret;

	// Read 10 bytes from the touchscreen at bus addr 0x04
	ret = i2c_master_recv(client, buf, EXC7200_LEN_FRAME);
	if (ret < 0)
		return ret;

	if (ret != EXC7200_LEN_FRAME)
		return -EIO;

	return 0;
}

static irqreturn_t exc7200_interrupt(int irq, void *dev_id)
{
	// Touchscreen has flagged us by pulling the interrupt line low on GPIO-004 (pin 7)

	// Set up some data structures
	struct exc7200_data *data = dev_id;
	struct input_dev *input = data->input;
	u8 *buf = data->buf;
	del_timer_sync(&data->timer);

	int ret;

	// Read from the touchscreen
	ret = exc7200_read_frame(data, buf);

	// Parse the second byte returned for the type of touch event
	// 131 (0x83) = Single touch event start (single finger MAKES contact with screen)
	// 130 (0x82) = Single touch event end (single finger BREAKS contact with screen)
	// 135 (0x87) = Double touch event start (two fingers MAKE contact with screen)
        // 134 (0x86) = Double touch event end (two fingers BREAK contact with screen)
	input_mt_report_slot_state(input, MT_TOOL_FINGER, true);

	switch (buf[1]) {
	        case EXC7200_ST1_EVENT:
			xa = abs(buf[2] | (buf[3] << 8))/8;
			ya = abs(buf[4] | (buf[5] << 8))/8;
			if ((tType == 0)){ /// Only update slot 0 with first finger pos if we're NOT in a two finger event

	                        input_mt_report_finger_count(input, 1);

				input_mt_slot(input, 0);
				input_mt_report_slot_state(input, MT_TOOL_FINGER, true);

			        input_report_abs(input, ABS_MT_POSITION_X, xa );
				input_report_abs(input, ABS_MT_POSITION_Y, ya );

		        	input_mt_sync_frame(input);
		        	input_sync(input);
			}
                	break;
	        case EXC7200_ST2_EVENT:
			tType = 0; // end two finger touch
                        input_mt_slot(input, 1);
			input_report_key(input, BTN_TOUCH, 0);
                        input_mt_report_slot_inactive(input);
                        input_mt_drop_unused(input);
                        input_mt_sync_frame(input);
                        input_sync(input);
                	break;
	        case EXC7200_MT1_EVENT:
			tType = 1;
			xb = abs(buf[2] | (buf[3] << 8));
                        yb = abs(buf[4] | (buf[5] << 8));
			w = max(xa, xb) - min(xa, xb);
			h = max(ya, yb) - min(ya, yb);
			p = int_sqrt( (w^2) + (h^2) );

			input_mt_report_finger_count(input, 2);
			input_mt_slot(input, 0);

//			input_report_key(input, BTN_TOUCH, 1);

                        input_report_abs(input, ABS_MT_TOUCH_MAJOR, w * SENSOR_AREA_UNIT);
                        input_report_abs(input, ABS_MT_TOUCH_MINOR, h * SENSOR_AREA_UNIT);
                        input_report_abs(input, ABS_MT_PRESSURE, p);
       	                input_report_abs(input, ABS_MT_POSITION_X, xa );
                        input_report_abs(input, ABS_MT_POSITION_Y, ya );

                        input_sync(input);

			input_mt_slot(input, 1);

		        input_report_abs(input, ABS_MT_TOUCH_MAJOR, (w * SENSOR_AREA_UNIT)*-1);
	       		input_report_abs(input, ABS_MT_TOUCH_MINOR, (h * SENSOR_AREA_UNIT)*-1);
			input_report_abs(input, ABS_MT_PRESSURE, p);
			input_report_abs(input, ABS_MT_POSITION_X, xb/8 );
			input_report_abs(input, ABS_MT_POSITION_Y, yb/8 );

  	        	input_sync(input);

	              	break;
		case EXC7200_MT2_EVENT:
			tType = 0; // end two finger touch
			p=0;
                        input_mt_slot(input, 1);
                        input_report_key(input, BTN_TOUCH, 0);
                        input_report_abs(input, ABS_MT_PRESSURE, 0);
                        input_mt_report_slot_inactive(input);
                        input_mt_sync_frame(input);
                        input_sync(input);
			break;
	}

	return IRQ_HANDLED;
}

static int exc7200_probe(struct i2c_client *client)
{
	// Initialisation routine.
	struct exc7200_data *data;
	struct input_dev *input;
	int error, max_xy;

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	data->info = device_get_match_data(&client->dev);
	if (!data->info) {
		enum eeti_dev_id eeti_dev_id =
			i2c_match_id(exc7200_id, client)->driver_data;
		data->info = &exc7200_info[eeti_dev_id];
	}
	init_completion(&data->wait_event);
	mutex_init(&data->query_lock);

	input = devm_input_allocate_device(&client->dev);
	if (!input)
		return -ENOMEM;

	data->input = input;
	input_set_drvdata(input, data);

	input->name = data->info->name;
	input->id.bustype = BUS_I2C;

	max_xy = data->info->max_xy;

	// dev, axis
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, SENSOR_RES_X, 4, 8);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, SENSOR_RES_Y, 4, 8);

        input_set_abs_params(input, ABS_MT_TOOL_X, 0, SENSOR_RES_X, 4, 8);
        input_set_abs_params(input, ABS_MT_TOOL_Y, 0, SENSOR_RES_Y, 4, 8);

        /* max value unknown, but major/minor axis
         * can never be larger than screen */
        input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, SENSOR_RES_X, 4, 8);
        input_set_abs_params(input, ABS_MT_TOUCH_MINOR, 0, SENSOR_RES_Y, 4, 8);

        input_set_abs_params(input, ABS_MT_ORIENTATION, 0, 1, 0, 0);
	input_set_capability(input, EV_KEY, BTN_TOUCH);

	touchscreen_parse_properties(input, true, &data->prop);

	error = input_mt_init_slots(input, EXC7200_NUM_SLOTS, INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);

	if (error)
		return error;

	error = input_register_device(input);
	if (error)
		return error;

	error = devm_request_threaded_irq(&client->dev, client->irq,
					  NULL, exc7200_interrupt, IRQF_ONESHOT,
					  client->name, data);
	if (error)
		return error;

	return 0;
}

static const struct i2c_device_id exc7200_id[] = {
	{ "exc7200", EETI_EXC7200 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, exc7200_id);

static struct i2c_driver exc7200_driver = {
	.driver = {
		.name	= "exc7200",
	},
	.id_table	= exc7200_id,
	.probe		= exc7200_probe,
};

module_i2c_driver(exc7200_driver);

MODULE_AUTHOR("Ahmet Inan <inan@distec.de>");
MODULE_DESCRIPTION("I2C connected EETI EXC7200 multiple touch controller driver");
MODULE_LICENSE("GPL v2");
