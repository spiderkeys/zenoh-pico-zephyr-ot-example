#include <zephyr.h>
#include <zenoh-pico.h>

#include <stdio.h>
#include <stdlib.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

void data_handler(const zn_sample_t *sample, const void *arg)
{
    const struct device *dev = (struct device*)arg;
    static bool led_is_on = false;

    gpio_pin_set(dev, PIN, (int)led_is_on);
    led_is_on = !led_is_on;

    printk(">> [Subscription listener] Received (%.*s, %.*s)\n",
           (int)sample->key.len, sample->key.val,
           (int)sample->value.len, sample->value.val);
}

void main(void)
{
    const struct device *dev;
	bool led_is_on = false;
    int ret;
    dev = device_get_binding(LED0);
	if (dev == NULL) {
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

    // Set initial state to off
    gpio_pin_set(dev, PIN, (int)led_is_on);

    char *uri = "/demo/example/**";

    sleep( 5 );

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make( "udp/fd10:9b50:100e:0:60df:9983:d08a:c29f:7474" ));

    zn_session_t *s = NULL;
    while( s == NULL )
    {
        printk( "Connecting!\n" );
        s = zn_open(config);
        printk( "Connected\n" );
        if (s == 0)
        {
            printk("Unable to open session!\n");

            sleep( 3 );
        }
    }
    
    // Start the read session session lease loops
    int rez = znp_start_read_task(s);
    if( rez == 0 )
    {
        printk( "Started read task\n" );
    }
    else{
        printk( "Failed read task %d\n", rez );
    }
    znp_start_lease_task(s);

    zn_subscriber_t *sub = zn_declare_subscriber(s, zn_rname(uri), zn_subinfo_default(), data_handler, dev );
    if (sub == 0)
    {
        printk("Unable to declare subscriber.\n");
        
    }

    printk("Awaiting data\n"); 
    while( true )
    {
        sleep( 1 );
    }

    zn_undeclare_subscriber(sub);
    zn_close(s);

    return;    
}
