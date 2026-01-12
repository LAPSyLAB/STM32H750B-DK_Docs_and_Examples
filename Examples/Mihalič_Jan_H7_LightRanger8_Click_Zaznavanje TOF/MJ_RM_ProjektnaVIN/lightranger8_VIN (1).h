// Descriptions are a guess!


#define LIGHTRANGER8_SOFT_RESET                                         0x0000

// If it holds a certain value then a newly measured value is ready to be read.
#define LIGHTRANGER8_GPIO_TIO_HV_STATUS                                 0x0031

// Controls the period between consecutive measurements
#define LIGHTRANGER8_SYSTEM_INTERMEASUREMENT_PERIOD                     0x006C

// Write a certain value into this register to clear the interrupt.
#define LIGHTRANGER8_SYSTEM_INTERRUPT_CLEAR                             0x0086

// Disable or enable ranging mode
#define LIGHTRANGER8_SYSTEM_MODE_START                                  0x0087

// Holds the measured distance
#define LIGHTRANGER8_RESULT_FINAL_CROSSTALK_CORRECTED_RANGE_MM_SD0      0x0096

// Holds a value related to the calibration of the oscillator
#define LIGHTRANGER8_RESULT_OSC_CALIBRATE_VAL                           0x00DE

// Holds model ID of the sensor
#define LIGHTRANGER8_IDENTIFICATION_MODEL_ID                            0x010F

// These registers configure the LightRanger8 sensor's distance measurement settings (SHORT, MEDIUM, LONG).
#define LIGHTRANGER8_RANGE_CONFIG_VCSEL_PERIOD_A                        0x0060
#define LIGHTRANGER8_RANGE_CONFIG_VCSEL_PERIOD_B						0x0063
#define LIGHTRANGER8_RANGE_CONFIG_VALID_PHASE_HIGH                      0x0069
#define LIGHTRANGER8_SD_CONFIG_WOI_SD0                                  0x0078
#define LIGHTRANGER8_SD_CONFIG_WOI_SD1                                  0x0079
#define LIGHTRANGER8_SD_CONFIG_INITIAL_PHASE_SD0                        0x007A
#define LIGHTRANGER8_SD_CONFIG_INITIAL_PHASE_SD1                        0x007B
