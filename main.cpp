/*
* author: Ryusuke Iwata
* date  : 2023/05/15
* note  : 
*/

#include "mbed.h"
#include <cstdint>
RawSerial pc(USBTX, USBRX, 9600);
SPI spi(p5, p6, p7);
DigitalOut cs(p8);
DigitalOut led(LED1);

/* define XLink variables */
/* 1.4 Terms and Definitions */
#define U32 uint32_t //!< define a unsigned 32 bit datatype
#define U16 uint16_t //!< define a unsigned 16 bit datatype
#define U8  uint8_t  //!< define a unsigned 8 bit datatype
#define S32 int32_t  //!< define a signed 32 bit datatype
#define S16 int16_t  //!< define a signed 16 bit datatype
#define S8  int8_t   //!< define a signed 8 bit datatype

/* 3.1 Overview on the Service Server Structures */ 
#define SERVICE_SERVER_PORT 51003
#define SERVICE_LIVE_TIME   600
/* an established TCP connection will be closed after N*10ms without actions */
/* service requests and response flags */
#define SERVICE_XMT_DATA_STOP 0x04000000 /* Stop the XmtData Transfer */
#define SERVICE_XMT_DATA_RUN  0x08000000 /* Run the XmtData Transfer after stop */
#define SERVICE_GET           0x10000000 /* read data request (GET) */
#define SERVICE_SET           0x20000000 /* write data request (SET) */
#define SERVICE_ACK           0x40000000 /* acknowledged request response (ACK) */
#define SERVICE_NAK           0x80000000 /* not acknowledged request */
/* service events */
/* read only = r; write only = w; read & write = rw */
#define SERVICE_IDLE           0x00000000 /* w */
#define SERVICE_DEVICE_CONF    0x00000001 /* rw */
#define SERVICE_FEATURE_SELECT 0x00000002 /* r */
#define SERVICE_VERSION        0x00000003 /* rw */
#define SERVICE_HOUSE_KEEP     0x00000004 /* r */
#define SERVICE_IP_ADR         0x00000009 /* w */
#define SERVICE_IP_ADR_SAVE    0x0000000A /* w */
#define SERVICE_START_RF_RCV   0x0000000E /* w */
#define SERVICE_RF_XMT_DATA    0x0000000F /* w */
#define SERVICE_RESET          0x009AE41B /* w */

/* 3 Service Server Protocol */
typedef struct
{
    /*----------------------- U32 boundary [0] ------------------------ Header start */
    U32 product_key[4]; //!< XLINK device specific key
    U32 service_event; //!< type of the service request
    /*----------------------- U32 boundary [5] ------------------------ Header end */
    U32 data; //!< max. 2056 byte */
} SERVICE_COMMON_STRUCT;

/* 3.2SPI-Messages */
typedef struct
{
    U16 length; //!< SPI-Message length in byte
    U8 ident; //!< SPI-Message identifier
    #define SPI_NODATA 0
    #define SPI_SERVICE 10
    #define SPI_DATA 11
    U8 from; //!< sender identification (currently not used)
//    #define MAX_DATA_SPI_MSG 2192 //!< max msg size (shorter is always possible)
    #define MAX_DATA_SPI_MSG 2192 //!< max msg size (shorter is always possible)
    U8 messageBuffer[MAX_DATA_SPI_MSG]; //!< array field for different SPI-Message data
} SPI_MSG_DATA_STRUCT;

typedef struct
{
/*----------------------- U32 boundary [0] -------------------------*/
    U32 product_key[4]; //!< XLINK device specific key
    U32 service_event; //!< type of the service request
/*----------------------- U32 boundary [5] -------------------------*/
} SERVICE_IDLE_STRUCT; /* size: 20 bytes (5 WORDs) */

// SERVICE_COMMON_STRUCT commonStr;
// SPI_MSG_DATA_STRUCT   spiDataStr;

#define SPI_START_BYTE 0x53504953 //means SPIS
#define SPI_END_BYTE   0x53504945 //means SPIE
// Product Key: 8a75ed7b-6cd6cfb8-81552300-42cc7396
#define PRODUCT_KEY_A  0x8a75ed7b
#define PRODUCT_KEY_B  0x6cd6cfb8
#define PRODUCT_KEY_C  0x81552300
#define PRODUCT_KEY_D  0x42cc7396
#define PRODUCT_KEY    8a75ed7b6cd6cfb88155230042cc7396

void littleEndian(uint32_t num) {
    unsigned char *ptr = (unsigned char *)&num;
    int i;
    for (i = 0; i < sizeof(uint32_t); i++) {
        pc.printf("%02X ", ptr[i]);
    }
    pc.printf("\r\n");
}
int combineHexData(unsigned int data1, unsigned int data2) {
    return (data1 << 8) | data2;
}

char cmd;
int main() {
    pc.printf("\r\n\r\n[ Start ]\r\n");
    uint32_t SPI_START_BYTE_LE, SERVICE_HOUSE_KEEP_GET_LE, SPI_END_BYTE_LE;
    uint8_t  spis_array[4], spie_array[4], le_spis_array[4], le_spie_array[4], length_array[2]; 

    // creating start byte "SPIS"
    uint32_t number   = SPI_START_BYTE;
    uint32_t combined = 0x00;
    pc.printf(">> littleEndian SPI_START_BYTE Start \r\n");
    pc.printf(">>>> SPI_START_BYTE   : 0x%x \r\n", number);
    for (int i = 0; i < sizeof(number); i++) {
        le_spis_array[i] = ((unsigned char *)&number)[i];
    }
    int be_num = 0;
    for (int i = sizeof(number) - 1; i >= 0; i--) {
        spis_array[be_num] = ((unsigned char *)&number)[i];
        be_num++;
    }
    for (int i = 0; i < sizeof(number); i++) {
        pc.printf(">>>>    spis_array[%d]: 0x%x\r\n", i, spis_array[i]);
        pc.printf(">>>> le_spis_array[%d]: 0x%x\r\n", i, le_spis_array[i]);
    }
    
    // creating end byte "SPIE"
    number   = SPI_END_BYTE;
    combined = 0x00;
    pc.printf("\r\n>> littleEndian SPI_END_BYTE Start \r\n");
    pc.printf(">>>> SPI_END_BYTE   : 0x%x \r\n", number);
    for (int i = 0; i < sizeof(number); i++) {
        le_spie_array[i] = ((unsigned char *)&number)[i];
    }
    be_num = 0;
    for (int i = sizeof(number) - 1; i >= 0; i--) {
        spie_array[be_num] = ((unsigned char *)&number)[i];
        be_num++;
    }
    for (int i = 0; i < sizeof(number); i++) {
        pc.printf(">>>>    spie_array[%d]: 0x%x\r\n", i, spie_array[i]);
        pc.printf(">>>> le_spie_array[%d]: 0x%x\r\n", i, le_spie_array[i]);
    }

    // creating survice hk massage
    pc.printf(">> Define SERVICE_HOUSE_KEEPING Start \r\n");
    pc.printf(">> Define SERVICE_HOUSE_KEEPING MSG Start \r\n"); 
    SPI_MSG_DATA_STRUCT survice_hk_msg = {0x0014, 0x0A, 0x00};
    // SPI_MSG_DATA_STRUCT survice_hk_msg = {0x0014, 0x00, 0x00};
    for (int i = 0; i < 2; i++) {
        length_array[i] = ((unsigned char *)&survice_hk_msg.length)[i];
    }
    pc.printf(">>>> length: 0x%x\r\n", length_array[0]);
    pc.printf(">>>> length: 0x%x\r\n", length_array[1]);
    pc.printf(">>>> ident　　: 0x%x\r\n", survice_hk_msg.ident);
    pc.printf(">>>> from　　　　: 0x%x\r\n", survice_hk_msg.from);

    U32 current_service_event = 0x10000004;
    SERVICE_IDLE_STRUCT survice_hk_str = {{PRODUCT_KEY_A, PRODUCT_KEY_B, PRODUCT_KEY_C, PRODUCT_KEY_D}, current_service_event};
    pc.printf(">>>> current_service_event          = 0x%08x\r\n", current_service_event);
    for (int i = 0; i < 4; i++) {
        pc.printf(">>>> survice_hk_str.product_key   = %x\r\n"    , survice_hk_str.product_key[i]);
    }
    pc.printf(">>>> survice_hk_str.service_event = %x\r\n"    , survice_hk_str.service_event);

    pc.printf("\r\n>> littleEndian SERVICE_HOUSE_KEEPING Start \r\n");
    pc.printf(">>>> littleEndian product_key Start \r\n");
    uint8_t le_product_key[16], le_service_event[4];
    number = survice_hk_str.product_key[0];
    for(int j = 0; j < 4; j ++){
        for (int i = 0; i < sizeof(survice_hk_str.product_key[j]); i++) {
        // for (int i = 0; i < 4; i++) {
            le_product_key[j*3 + i + j] = ((unsigned char *)&survice_hk_str.product_key[j])[i];
        }
    }
    for(int j = 0; j < 4; j ++){
        // for (int i = 0; i < sizeof(survice_hk_str.product_key[j]); i++) {
        for (int i = 0; i < 4; i++) {
            pc.printf(">>>> le_product_key[%d]: 0x%x\r\n", j*3 + i + j, le_product_key[j*3 + i + j]);
        }
    }
    pc.printf("\r\n>>>> littleEndian service_event Start \r\n");
    number = survice_hk_str.service_event;
    for (int i = 0; i < sizeof(survice_hk_str.service_event); i++) {
        le_service_event[i] = ((unsigned char *)&number)[i];
    }
    for (int i = 0; i < sizeof(survice_hk_str.service_event); i++) {
        pc.printf(">>>> le_service_event[%d]: 0x%x\r\n", i, le_service_event[i]);
    }


    

    // spi.format(8,3);
    spi.frequency(1000000);
    cs = 1;
    pc.printf("\r\n\r\n>> Start SPI Sending Program\r\n");
    while(1){
        pc.printf(">> Hit Any Key for sending\r\n\r\n");
        cmd = pc.getc();
        if(cmd){
            pc.printf(">>>> SPI Sending Data\r\n");
            cs = 0;
            // pc.printf("\r\n>> Send littleEndian SPI_START_BYTE Start \r\n"); // write spis in little endian
            for (int i = 0; i < sizeof(le_spis_array)/sizeof(le_spis_array[0]); i++) {
                spi.write(le_spis_array[i]);
                // pc.printf(">>>> le_spis_array[%d]: 0x%x\r\n", i, le_spis_array[i]);

                // spi.write(spis_array[i]);
                // pc.printf(">>>> spis_array[%d]: 0x%x\r\n", i, spis_array[i]);
            }

            // pc.printf("\r\n>> Send length, ident, from\r\n"); // write spis 
            spi.write(length_array[1]);
            spi.write(length_array[0]);
            spi.write(survice_hk_msg.ident);
            spi.write(survice_hk_msg.from);
            // pc.printf(">>>> survice_hk_msg.length: 0x%x\r\n", length_array[0]);
            // pc.printf(">>>> survice_hk_msg.length: 0x%x\r\n", length_array[1]);
            // pc.printf(">>>> survice_hk_msg.ident: 0x%x\r\n", survice_hk_msg.ident);
            // pc.printf(">>>> survice_hk_msg.from: 0x%x\r\n", survice_hk_msg.from);

            // pc.printf("\r\n>> Send littleEndian product_key Start \r\n"); // write product_key in little endian
            for (int i = 0; i < sizeof(le_product_key)/sizeof(le_product_key[0]); i++) {
                spi.write(le_product_key[i]);
                // pc.printf(">>>> le_product_key[%d]: 0x%x\r\n", i, le_product_key[i]);
            }

            // pc.printf("\r\n>> Send littleEndian service_event Start \r\n"); // write product_key in little endian
            for (int i = 0; i < sizeof(le_service_event)/sizeof(le_service_event[0]); i++) {
                spi.write(le_service_event[i]);
                // pc.printf(">>>> le_service_event[%d]: 0x%x\r\n", i, le_service_event[i]);
            }

            // pc.printf("\r\n>> Send littleEndian SPI_END_BYTE Start \r\n"); // write spie in little endian
            for (int i = 0; i < sizeof(le_spie_array)/sizeof(le_spie_array[0]); i++) {
                spi.write(le_spie_array[i]);
                // pc.printf(">>>> le_spie_array[%d]: 0x%x\r\n", i, le_spie_array[i]);

                // spi.write(spie_array[i]);
                // pc.printf(">>>> spie_array[%d]: 0x%x\r\n", i, spie_array[i]);
            }

            cs = 1;
            pc.printf(">>>> SPI Sending Data END\r\n\r\n");
            led = !led;
        }
    }
}
