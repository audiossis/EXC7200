#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <string.h>
#include <wiringPi.h>

//using namespace std;

const int INPUT_PIN = 7;
int i2c_fd = 0;
char buf[10] = {0};
int fd = 0;
char disp_buf[20];

int xa;
int ya;
int za;

int xb;
int yb;
int zb;


static void setup_abs(int fd, int type, int min, int max, int res)
{
    struct uinput_abs_setup abs = {
        .code = type,
        .absinfo = {
            .minimum = min,
            .maximum = max,
            .resolution = res
        }
    };

    ioctl(fd, UI_ABS_SETUP, &abs);
}

static void uinputInit(int fd, int width, int height, int dpi)
{
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);


    /* the ioctl UI_ABS_SETUP enables these automatically, when appropriate:
        ioctl(fd, UI_SET_ABSBIT, ABS_X);
        ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    */

    struct uinput_setup device = {
        .id = {
            .bustype = BUS_USB
        },
        .name = "Emulated Absolute Positioning Device"
    };

    ioctl(fd, UI_DEV_SETUP, &device);

    setup_abs(fd, ABS_X, 0, width, dpi);
    setup_abs(fd, ABS_Y, 0, height, dpi);

    ioctl(fd, UI_DEV_CREATE);

    /* give time for device creation */
    sleep(1);
}

static void emit(int fd, int type, int code, int value)
{
    struct input_event ie = {
        .type = type,
        .code = code,
        .value = value
    };

    write(fd, &ie, sizeof ie);
}

void copyArr(char *arr1, char *arr2, int n) {
    for (int i = 0+n; i < 10+n; i++) {

        // Copy each element by dereferencing
        *(arr2 + i) = *(arr1 + i-n);
    }
}

void i2cInt() {
    while (digitalRead(INPUT_PIN) == LOW) {
	// Invoke system("clear")
//	system("clear");

        read(i2c_fd, buf, 10);

        if (buf[1] == 131) {
	    copyArr(buf, disp_buf, 0);
//    	    printf("\rBuf: %3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
            xa = abs((buf[2] | (buf[3] << 8))/8);
            ya = abs((buf[4] | (buf[5] << 8))/8);
            za = abs((buf[6] | (buf[7] << 8))/8);

//            printf("Mouse Down: X: %d Y: %d\n", buf[3], buf[5]);
            struct input_event ev;
            memset(&ev, 0, sizeof(ev));

            /* input is zero-based, but event positions are one-based */
            emit(fd, EV_ABS, ABS_X, buf[3]);
            emit(fd, EV_ABS, ABS_Y, buf[5]);

            emit(fd, EV_KEY, BTN_LEFT, 1);
            emit(fd, EV_SYN, SYN_REPORT, 0);
        }
        else if (buf[1] == 130) {
//            printf("Mouse Up\n");
            struct input_event ev;
            memset(&ev, 0, sizeof(ev));
            emit(fd, EV_KEY, BTN_LEFT, 0);
            emit(fd, EV_SYN, SYN_REPORT, 0);
        }
	else if (buf[1] == 135) {
	    copyArr(buf, disp_buf, 10);
            xb = abs((buf[2] | (buf[3] << 8))/8);
            yb = abs((buf[4] | (buf[5] << 8))/8);
            zb = abs((buf[6] | (buf[7] << 8))/8);

//	    printf("\nBuf: %3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
//	    printf("Two point touch begin\n");
        }
        else if (buf[1] == 134) {
//		printf("Two point touch end\n");
        }
	else {
//		printf("CMD: %d\n", buf[1]);
	}
//	printf("\rBuf: %3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d\t\t%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d:%3d", disp_buf[0],disp_buf[1],disp_buf[2],disp_buf[3],disp_buf[4],disp_buf[5],disp_buf[6],disp_buf[7],disp_buf[8],disp_buf[9],disp_buf[10],disp_buf[11],disp_buf[12],disp_buf[13],disp_buf[14],disp_buf[15],disp_buf[16],disp_buf[17],disp_buf[18],disp_buf[19]);
	printf("\rxa:%3d ya:%3d za:%3d\t\txb:%3d yb:%3d zb:%3d", xa, ya, za, xb, yb, zb);
    }
};

int main() {
    setenv("XDG_RUNTIME_DIR", "/tmp/runtime-root", 1);

    // Initialize uinput device
    fd = open("/dev/uinput", O_RDWR | O_NONBLOCK);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

    /* These values are very device specific */
    int w = 126;
    int h = 126;
    int d = 200;
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Mouse");

    printf("Initializing device screen map as %dx%d @ %ddpi\n", w, h, d);
    uinputInit(fd, w, h, d);

    uidev.absmax[ABS_X] = 255;
    uidev.absmax[ABS_Y] = 255;
    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    printf("Opening i2c bus.\n");
    i2c_fd = open("/dev/i2c-1", O_RDONLY);
    ioctl(i2c_fd, I2C_SLAVE, 0x04);
    int buf[8];

    printf("Setup interrupt pin.\n");
    wiringPiSetup();
    pinMode(INPUT_PIN, INPUT);
    wiringPiISR(INPUT_PIN, INT_EDGE_FALLING, i2cInt);
    system("clear");

    while (1) {
        usleep(5);
    }

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    close(i2c_fd);
    return 0;
}

