#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/gpio.h> // Required for the GPIO functions
#include <asm/uaccess.h>

// #error Are we building this?

#define MY_DEVICE_FILE "matrix"

/* GPIO Pins Definition */
#define RED1_PIN 8 // UPPER
#define GREEN1_PIN 80
#define BLUE1_PIN 78
#define RED2_PIN 76 // LOWER
#define GREEN2_PIN 79
#define BLUE2_PIN 74
#define CLK_PIN 73   // Arrival of each data
#define LATCH_PIN 75 // End of a row of data
#define A_PIN 72     // Row select
#define B_PIN 77
#define C_PIN 70

#define DELAY_IN_US 1000
/* LED Screen Values */
static int screen[32][16];

//look up datasheet
static void export_pin(int pin_num)
{
    // Export the gpio pins
    gpio_request(pin_num, "sysfs");
    gpio_direction_output(pin_num, true);
    gpio_export(pin_num, true);

    return;
}

static void unexport_pins(void)
{
    gpio_unexport(RED1_PIN);
    gpio_unexport(GREEN1_PIN);
    gpio_unexport(BLUE1_PIN);

    gpio_unexport(RED2_PIN);
    gpio_unexport(GREEN2_PIN);
    gpio_unexport(BLUE2_PIN);

    gpio_unexport(CLK_PIN);
    gpio_unexport(LATCH_PIN);

    gpio_unexport(A_PIN);
    gpio_unexport(B_PIN);
    gpio_unexport(C_PIN);
}

static void unregister_pins(void)
{

    unexport_pins();

    gpio_free(RED1_PIN);
    gpio_free(GREEN1_PIN);
    gpio_free(BLUE1_PIN);

    // Lower led
    gpio_free(RED2_PIN);
    gpio_free(GREEN2_PIN);
    gpio_free(BLUE2_PIN);

    // Timing
    gpio_free(CLK_PIN);
    gpio_free(LATCH_PIN);

    // Row Select
    gpio_free(A_PIN);
    gpio_free(B_PIN);
    gpio_free(C_PIN);
}

/**
 * init_pins
 * Setup the pins used by the led matrix, by exporting and set the direction to out
 */
static void init_pins(void)
{
    // !Upper led
    export_pin(RED1_PIN);
    export_pin(GREEN1_PIN);
    export_pin(BLUE1_PIN);

    // Lower led
    export_pin(RED2_PIN);
    export_pin(GREEN2_PIN);
    export_pin(BLUE2_PIN);

    // Timing
    export_pin(CLK_PIN);
    export_pin(LATCH_PIN);

    // Row Select
    export_pin(A_PIN);
    export_pin(B_PIN);
    export_pin(C_PIN);

    return;
}
/** 
 *  bang_clock
 *  Generate the clock pins
 */
static void bang_clock(void)
{
    // Bit-bang the clock gpio
    // Notes: Before program writes, must make sure it's on the 0 index
    gpio_set_value(CLK_PIN, 1);
    gpio_set_value(CLK_PIN, 0);

    return;
}

/**
 *  bang_latch
 *  Generate the latch pins
 */
static void bang_latch(void)
{
    gpio_set_value(LATCH_PIN, 1);
    gpio_set_value(LATCH_PIN, 0);

    return;
}

/**
 *  ledMatrix_bitsFromInt
 *  Convert integer passed on into bits and put in array
 *  @params:
 *      int * arr: pointer to array to be filled with bits
 *      int input: integer to be converted to bits
 */
static void int_to_bits(int input, int *arr)
{
    arr[0] = input & 1;

    arr[1] = input & 2;
    arr[1] = arr[1] >> 1;

    arr[2] = input & 4;
    arr[2] = arr[2] >> 2;

    return;
}

/**
 *  set_row
 *  Set LED Matrix row
 *  @params:
 *      int rowNum: the rowNumber to be inputted to row pins
 */
static void set_row(int row_num)
{
    // Convert rowNum single bits from int
    int bits[3] = {0, 0, 0};
    int_to_bits(row_num, bits);

    // Write on the row pins
    gpio_set_value(A_PIN, bits[0]);
    gpio_set_value(B_PIN, bits[1]);
    gpio_set_value(C_PIN, bits[2]);

    return;
}

/**
 *  set_colour_top
 *  Set the colour of the top part of the LED
 *  @params:
 *      int colour: colour to be set
 */
static void set_colour_top(int colour)
{
    int bits[3] = {0, 0, 0};
    int_to_bits(colour, bits);

    gpio_set_value(RED1_PIN, bits[0]);
    gpio_set_value(GREEN1_PIN, bits[1]);
    gpio_set_value(BLUE1_PIN, bits[2]);

    return;
}

/**
 *  set_colour_bottom
 *  Set the colour of the bottom part of the LED
 *  @params:
 *      int colour: colour to be set
 */
static void set_colour_bottom(int colour)
{
    int bits[3] = {0, 0, 0};
    int_to_bits(colour, bits);

    gpio_set_value(RED2_PIN, bits[0]);
    gpio_set_value(GREEN2_PIN, bits[1]);
    gpio_set_value(BLUE2_PIN, bits[2]);

    return;
}
/**
 *  refresh_matrix
 *  Fill the LED Matrix with the respective pixel colour
 */
static void refresh_screen(void)
{
    int rowNum;
    for (rowNum = 0; rowNum < 8; rowNum++)
    {
        int colNum;
        set_row(rowNum);
        for (colNum = 0; colNum < 32; colNum++)
        {
            set_colour_top(screen[colNum][rowNum]);
            set_colour_bottom(screen[colNum][rowNum + 8]);
            bang_clock();
        }
        //does latch need to be clocked?
        bang_latch();
        //usleep_range is prefered over msleep for values <20ms
        // usleep_range(DELAY_IN_US, DELAY_IN_US);
    }

    return;
}

// static void set_pixel(int x, int y, int colour)
// {
//     screen[y][x] = colour;

//     return;
// }

/******************************************************
 * Callbacks
 ******************************************************/
static ssize_t write(struct file *file,
                     const char *buff, size_t count, loff_t *ppos)
{
    // memset(screen, 0, sizeof(screen));
    if (copy_from_user(screen, buff, count))
    {
        return -EFAULT;
    }

    // printk(KERN_INFO "Read %p, with size %d\n", screen, count);
    refresh_screen();

    // int i;
    // printk(KERN_INFO "Starting program!\n");

    // for (i = 0; i < 32; i++)
    // {
    //     int j;
    //     int k;
    //     memset(screen, 0, sizeof(screen));
    //     printk(KERN_INFO "Starting set_pixel loop!\n");
    //     // for (j = 7; j < 8; j++)
    //     {
    //         set_pixel(2, i, 1);
    //     }

    //     printk(KERN_INFO "Starting refresh loop!\n");
    //     for (k = 0; k < 250; k++)
    //     {
    //         refresh_screen();
    //     }
    // }

    // printk(KERN_INFO "Done!\n");
    return count;
}

/******************************************************
 * Misc support
 ******************************************************/
struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = write,
};

static struct miscdevice matrix_driver = {
    .minor = MISC_DYNAMIC_MINOR, // Let the system assign one.
    .name = MY_DEVICE_FILE,      // /dev/.... file.
    .fops = &fops                // Callback functions.
};

static int __init matrix_init(void)
{
    int ret;
    printk(KERN_INFO "----> matrix driver init(): file /dev/%s. JAKSHDJKAHSKDJHKJASD NEW STUFF\n", MY_DEVICE_FILE);

    ret = misc_register(&matrix_driver);
    init_pins();

    return ret;
}

static void __exit matrix_exit(void)
{
    printk(KERN_INFO "<---- matrix driver exit().\n");
    unregister_pins();
    misc_deregister(&matrix_driver);
}

module_init(matrix_init);
module_exit(matrix_exit);

MODULE_AUTHOR("Scott Plummer");
MODULE_DESCRIPTION("An LED driver");
MODULE_LICENSE("GPL"); // Important to leave as GPL.
