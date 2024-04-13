#include"main.h"

void OTG_FS_IRQHandler(void)
{
	tud_int_handler(0); // 0 means FS
}

static void usb_device_task(void *param)
{
	(void) param;
	tud_init(0); // FS init
	while(true) {
		tud_task();
	}
}

enum
{
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

static void joystick_init(void)
{
	LL_GPIO_InitTypeDef gpioc = {
		.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3,
		.Mode = LL_GPIO_MODE_ANALOG,
		.Pull = LL_GPIO_PULL_NO
	};
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	LL_GPIO_Init(GPIOC, &gpioc);

	LL_ADC_InitTypeDef adc_init = {
		.Resolution = LL_ADC_RESOLUTION_8B,
		.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT,
		.SequencersScanMode = LL_ADC_SEQ_SCAN_ENABLE
	};
	LL_ADC_CommonInitTypeDef adc_common_init = {
		.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV8,
		.Multimode = LL_ADC_MULTI_INDEPENDENT
	};
	LL_ADC_INJ_InitTypeDef adc_inj_init = {
		.TriggerSource = LL_ADC_INJ_TRIG_SOFTWARE,
		.SequencerLength = LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS,
		.SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_1RANK,
		.TrigAuto = LL_ADC_INJ_TRIG_INDEPENDENT
	};
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);

	LL_ADC_Init(ADC1, &adc_init);
	LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &adc_common_init);
	LL_ADC_INJ_Init(ADC1, &adc_inj_init);

	LL_ADC_SetSequencersScanMode(ADC1, LL_ADC_SEQ_SCAN_ENABLE);
	LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_1, LL_ADC_CHANNEL_12);
	LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_2, LL_ADC_CHANNEL_13);
	// not needed
	LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_12, LL_ADC_SAMPLINGTIME_480CYCLES);
	LL_ADC_Enable(ADC1);
	LL_ADC_INJ_StartConversionSWStart(ADC1);

	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
}

static int8_t get_delta_by_adc_val(int val)
{
	if (val < 32)
		return -3;
	if (val < 64)
		return -2;
	if (val < 96)
		return -1;
	if (val < 160)
		return 0;
	if (val < 192)
		return 1;
	if (val < 224)
		return 2;
	return 3;
}
static void usb_hid_task(void *param)
{
	uint8_t report_id = 0;
	uint8_t button = 0;
	int8_t delta_x = 0;
	int8_t delta_y = 0;
	int adc_x_val = 0;
	int adc_y_val = 0;
	uint8_t vertical = 0;
	uint8_t horizontal = 0;

	joystick_init();
	//vTaskDelay(pdMS_TO_TICKS(50));
	while (1) {
		// printf("reading adc!\r\n");
		adc_x_val = ADC1->JDR1;
		adc_y_val = ADC1->JDR2;
		delta_x = get_delta_by_adc_val(adc_x_val);
		delta_y = get_delta_by_adc_val(adc_y_val);
		//printf("x: %d, y: %d\r\n", adc_x_val, adc_y_val);
		tud_hid_n_mouse_report(ITF_NUM_HID, report_id, button, delta_x, delta_y, vertical, horizontal);
		LL_ADC_INJ_StartConversionSWStart(ADC1);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void usb_device_init(void) {
	LL_GPIO_InitTypeDef usb_pins = {
		.Pin = LL_GPIO_PIN_11 | LL_GPIO_PIN_12,
		.Mode = LL_GPIO_MODE_ALTERNATE,
		.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
		.OutputType = LL_GPIO_OUTPUT_PUSHPULL,
		.Pull = LL_GPIO_PULL_NO,
		.Alternate = LL_GPIO_AF_10
	};
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_GPIO_Init(GPIOA, &usb_pins);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	NVIC_SetPriority(OTG_FS_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_OTGFS);


	xTaskCreate(usb_device_task, "usbd", 256, NULL, USBD_TASK_PRIORITY, NULL);
	xTaskCreate(usb_hid_task, "usbhid", 256, NULL, USB_HID_TASK_PRIORITY, NULL);
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // This example doesn't use multiple report and report ID
  (void) itf;
  (void) report_id;
  (void) report_type;

  // echo back anything we received from host
  tud_hid_report(0, buffer, bufsize);
}

/* ------- USB Descriptors below --------- */

/* #define USB_VID 0xCafe
#define USB_BCD 0x0200
#define USB_PID	0x0222 */
/* #define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
		 _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )
 */
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = 0x0,
    .bDeviceSubClass    = 0x0,
    .bDeviceProtocol    = 0x0,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x0483,
    .idProduct          = 0x3748,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}


uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_MOUSE()
};

// invoked when received get hid report descriptor
// application return pointer to descriptor
// descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
  (void) itf;
  return desc_hid_report;
}

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID   0x01

uint8_t const desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // Interface number, string index, protocol, report descriptor len, In address, size & polling interval
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_report), 0x80 | EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 10)
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "Misha molodets!",                     // 1: Manufacturer
  "STM32F407VET6 of Misha",              // 2: Product
  NULL,                          // 3: Serials will use unique ID if possible
};

static uint16_t _desc_str[32 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  size_t chr_count;

  switch ( index ) {
    case STRID_LANGID:
      memcpy(&_desc_str[1], string_desc_arr[0], 2);
      chr_count = 1;
      break;

    /* case STRID_SERIAL:
      chr_count = board_usb_get_serial(_desc_str + 1, 32);
      break;
 */
    default:
      // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
      // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

      if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;

      const char *str = string_desc_arr[index];

      // Cap at max char
      chr_count = strlen(str);
      size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
      if ( chr_count > max_count ) chr_count = max_count;

      // Convert ASCII string into UTF-16
      for ( size_t i = 0; i < chr_count; i++ ) {
        _desc_str[1 + i] = str[i];
      }
      break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}
