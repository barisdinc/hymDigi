#include <cpu/irq.h>
#include <cfg/debug.h>

#include <net/afsk.h>
#include <net/ax25.h>

#include <drv/ser.h>
#include <drv/timer.h>

#include <stdio.h>
#include <string.h>
//#include <avr/io.h>
#include  <algo/crc_ccitt.h>
//export PATH=/home/baris/PROGRAMMING/arduino-1.8.13/hardware/tools/avr/bin/:$PATH
static Afsk afsk; 
static AX25Ctx ax25;
static Serial ser;

//#define EEMEM __attribute__((section(".eeprom")))
#define ADC_CH 0

//EEPROM function without using header file
//uint8_t eeprom_read_byte (const uint8_t *__p) __ATTR_PURE__;
//void eeprom_write_byte (uint8_t *__p, uint8_t __value);
void eeprom_read_block (void *__dst, const void *__src, size_t __n);
void eeprom_write_block (const void *__src, void *__dst, size_t __n);

typedef struct 
{
    char    EE_MYCALL[7];
    uint8_t EE_MYCALL_SSID;

    uint16_t EE_PREAMBLE;
    uint8_t  EE_BEACON_MODE; //WIDE1-1, WIDE2-1

    uint8_t  EE_APRS_BEACON_ENABLED;
    char     EE_APRS_BEACON_MSG[80];
    uint16_t EE_APRS_BEACON_INTERVAL;

    uint8_t  EE_HYMTR_BEACON_ENABLED;
    char     EE_HYMTR_BEACON_MSG[80];
    uint16_t EE_HYMTR_BEACON_INTERVAL;

    uint8_t  EE_TELEM_BEACON_ENABLED;
    char     EE_TELEM_BEACON_MSG[80];
    uint16_t EE_TELEM_BEACON_INTERVAL;

    uint8_t  EE_ADVERT_BEACON_ENABLED;
    char     EE_ADVERT_BEACON_MSG[80];
    uint16_t EE_ADVERT_BEACON_INTERVAL;

    uint16_t EE_CRC_VALUE;
} Aprs_Configuration_t;

Aprs_Configuration_t APRS_Configuration;

//static const char MYCALL[] = "YM2KDZ";
//static const char MYCALL[] = "YM5KRM";
//static const char MYCALL[] = "YM7KA";
//static const char MYCALL[] = "YM2KDZ";
//static uint8_t MYCALL_SSID = 3;
//static uint8_t MYCALL_SSID = 0;


//#define APRS_BEACON_MSG    "!4044.56N/03100.64E#PHG2820/DARD81 DUZCE DIGIPEATER ISTASYONU 750m" //"!4100.53N/02806.48E#>TA-APRS 016" //TA7W 
//#define APRS_BEACON_ENABLED  1
//#define APRS_BEACON_MSG    "!4100.53N/02806.48E#>TA-APRS - AKU xx.x V - 014"
//#define APRS_BEACON_MSG    "!3723.47N/03308.36E#AKRAD KARAMAN TEMSILCILIGI DIGIPEATER ISTASYONU 2250M"
//#define APRS_BEACON_INTERVAL 1*60
//#define APRS_BEACON_MSG    "!4049.41N/03106.43E#>TA-APRS DUZCE ATA-R"
//#define APRS_BEACON_MSG    "!4100.19N/03942.85E#>TA-APRS TRABZON ATA-R"
//#define APRS_BEACON_MSG    "!4044.56N/03100.65E#DUZCE DARD DIGI 600m"

//#define HYMTR_BEACON_ENABLED 1
//#define HYMTR_BEACON_MSG   ">hymDG ATA-R digipeater v1"
//#define HYMTR_BEACON_MSG   ">145.650Mhz   TX-RX 88.5   -600khz (NARROW)"
//#define HYMTR_BEACON_INTERVAL 2*60
//#define TELEM_BEACON_MSG   "!4044.56N/03100.65E#AKU xx.xV"

//#define TELEM_BEACON_ENABLED 0
//#define TELEM_BEACON_MSG   "!4044.56N/03100.64E#PHG2820/DARD81 DUZCE DIGIPEATER ISTASYONU 750m" //TA7W
//#define TELEM_BEACON_MSG   "!4044.56N/03100.64E#AKU xx.xV" //TA7W
//#define TELEM_BEACON_MSG   "!3723.47N/03308.36E#AKRAD KARAMAN TEMSILCILIGI DIGIPEATER ISTASYONU 2250m" //TA7W
//#define TELEM_BEACON_INTERVAL 3*60

//#define ADVERT_BEACON_ENABLED 0
//#define ADVERT_BEACON_MSG  ">TA-APRS 81 ILE APRS"
//#define ADVERT_BEACON_MSG  "!3723.47N/03308.36E#AKRAD KARAMAN TEMSILCILIGI DIGIPEATER ISTASYONU 2250 m"
//#define ADVERT_BEACON_INTERVAL 4*60


//#define APRS_BEACON_TIME (3 * 60)
//#define APRS_BEACON_TIME (60)

#define CALL_BERTOS_APRS "apatar" //

void eeprom_write_config(void)
{
    kfile_printf(&ser.fd, "Re-writing config...");
    eeprom_write_block (&APRS_Configuration, 0, sizeof(APRS_Configuration)); //Write configuration to beginning of eeprom
    timer_delay(100);
    kfile_printf(&ser.fd, "done\r\n");
}

void eeprom_read_config(void)
{
    kfile_printf(&ser.fd, "\r\n");
    kfile_printf(&ser.fd, "Reading Config...");
    eeprom_read_block(&APRS_Configuration, 0, sizeof(APRS_Configuration)); //Read Configuration from beginnning of eeprom
    timer_delay(100);
    kfile_printf(&ser.fd, "done\r\n");
   // if (APRS_Configuration.EE_CRC_VALUE!=crc_ccitt(0xFFFF, &APRS_Configuration, sizeof(APRS_Configuration)-2))
    {
        kfile_printf(&ser.fd, "CRC Failure\r\n");
        //CRC failed, fill EEPROM with default values
        strcpy(APRS_Configuration.EE_MYCALL, "YM6KGL\0");//"TA7W\0");
        APRS_Configuration.EE_MYCALL_SSID = 0;

        APRS_Configuration.EE_PREAMBLE = 300;
        APRS_Configuration.EE_BEACON_MODE = 1;

        APRS_Configuration.EE_APRS_BEACON_ENABLED = 1;
        strcpy(APRS_Configuration.EE_APRS_BEACON_MSG, "!4103.15N/03343.00E#PHG2920/CANKIRI ILGAZ DAGI TRT ISTASYONU 2064m             \0");//"Konfigurayon yok/Configure\0");
        APRS_Configuration.EE_APRS_BEACON_INTERVAL = 15*60;

        APRS_Configuration.EE_HYMTR_BEACON_ENABLED = 1;
        strcpy(APRS_Configuration.EE_HYMTR_BEACON_MSG, ">hymDG ATA-R digipeater v1.1a                                                \0");
        APRS_Configuration.EE_HYMTR_BEACON_INTERVAL = 30*60;

        APRS_Configuration.EE_TELEM_BEACON_ENABLED = 0;
        strcpy(APRS_Configuration.EE_TELEM_BEACON_MSG,"50 saniye                                                                     \0");
        APRS_Configuration.EE_TELEM_BEACON_INTERVAL = 50;

        APRS_Configuration.EE_ADVERT_BEACON_ENABLED = 0;
        strcpy(APRS_Configuration.EE_ADVERT_BEACON_MSG,"55 saniye                                                                    \0");
        APRS_Configuration.EE_ADVERT_BEACON_INTERVAL = 55;

        APRS_Configuration.EE_CRC_VALUE = crc_ccitt(0xFFFF, &APRS_Configuration, sizeof(APRS_Configuration)-2);
        eeprom_write_config();
    }
}

void print_digi_config(void)
{
    
    //kfile_printf(&ser.fd, "\n\nhymDiGi Configuration\r\n");
//    kfile_printf(&ser.fd, "%s-%d [P:%d] [M:%d]\r\n", APRS_Configuration.EE_MYCALL,                  
//                                                            APRS_Configuration.EE_MYCALL_SSID,     
//                                                            APRS_Configuration.EE_PREAMBLE,        
//                                                            APRS_Configuration.EE_BEACON_MODE);
    //kfile_printf(&ser.fd, "P:%d\r\n", APRS_Configuration.EE_PREAMBLE);
    //kfile_printf(&ser.fd, "Mode : %d\r\n", APRS_Configuration.EE_BEACON_MODE);
    //kfile_printf(&ser.fd, "APRS En : %c\r\n", APRS_Configuration.EE_APRS_BEACON_ENABLED==1?'Y':'N');

    //kfile_printf(&ser.fd, "APRS Msg : %d\r\n", strlen(APRS_Configuration.EE_APRS_BEACON_MSG));
    //kfile_printf(&ser.fd, "A : %s\r\n", APRS_Configuration.EE_APRS_BEACON_MSG);

    //kfile_printf(&ser.fd, "APRS In : %ss\r\n", APRS_Configuration.EE_APRS_BEACON_INTERVAL);
    //kfile_printf(&ser.fd, "HYMTR En : %c\r\n", APRS_Configuration.EE_HYMTR_BEACON_ENABLED==1?'Y':'N');
    //kfile_printf(&ser.fd, "HYMTR Msg : %s\r\n", APRS_Configuration.EE_HYMTR_BEACON_MSG);
    //kfile_printf(&ser.fd, "HYMTR In : %ss\r\n", APRS_Configuration.EE_HYMTR_BEACON_INTERVAL);
    //kfile_printf(&ser.fd, "TELEM En : %c\r\n", APRS_Configuration.EE_TELEM_BEACON_ENABLED==1?'Y':'N');
    //kfile_printf(&ser.fd, "TELEM Msg : %s\r\n", APRS_Configuration.EE_TELEM_BEACON_MSG);
    //kfile_printf(&ser.fd, "TELEM In : %ss\r\n", APRS_Configuration.EE_TELEM_BEACON_INTERVAL);
    //kfile_printf(&ser.fd, "ADVERT En : %c\r\n", APRS_Configuration.EE_ADVERT_BEACON_ENABLED==1?'Y':'N');
    //kfile_printf(&ser.fd, "ADVERT Msg : %s\r\n", APRS_Configuration.EE_ADVERT_BEACON_MSG);
    //kfile_printf(&ser.fd, "ADVERT In : %ss\r\n", APRS_Configuration.EE_ADVERT_BEACON_INTERVAL);




}



static void message_callback(struct AX25Msg *msg)
{
    if (PORTB & 0x08) 
    {
        return;
    }
    int i, k;

    static AX25Call tmp_path[AX25_MAX_RPT + 2];
    static uint8_t tmp_path_size;
    //static const char* relay_calls[] = {"RELAY\x0", "WIDE\x0\x0", "TRACE\x0", 0};
    static const char* relay_calls[] = {"WIDE\x0\x0", 0};


#if 1
    kfile_printf(&ser.fd, "\n\nSRC[%.6s-%d], DST[%.6s-%d]\r\n", msg->src.call, msg->src.ssid, msg->dst.call, msg->dst.ssid);

    for (i = 0; i < msg->rpt_cnt; i++)
        kfile_printf(&ser.fd, "via: [%.6s-%d%s]\r\n", msg->rpt_lst[i].call, msg->rpt_lst[i].ssid, msg->rpt_lst[i].h_bit?"*":"");

    kfile_printf(&ser.fd, "DATA: %.*s\r\n", msg->len, msg->info);
#endif

    if (1) {
        uint8_t repeat = 0;
        uint8_t is_wide = 0;//, is_trace = 0;

        msg->dst.ssid = 0; 
        msg->dst.h_bit = 0; //h_bit high is transmission
        //memcpy(msg->dst.call, CALL_BERTOS_APRS, 6);

        tmp_path[0] = msg->dst;
        tmp_path[1] = msg->src;

        for (i = 0; i < msg->rpt_cnt; ++i)
            tmp_path[i + 2] = msg->rpt_lst[i];
        tmp_path_size = 2 + msg->rpt_cnt;

        for (i = 2; i < tmp_path_size; ++i) {
            if (!tmp_path[i].h_bit) {
                AX25Call* c = &tmp_path[i];

                if ((memcmp(tmp_path[i].call, APRS_Configuration.EE_MYCALL, 6) == 0) && (tmp_path[i].ssid == APRS_Configuration.EE_MYCALL_SSID))
                {
                    repeat = 0;
                    break;
                }

                for (k=0; relay_calls[k]; ++k)
                {
                    if (memcmp(relay_calls[k], c->call, 6) == 0) {
                        repeat = 1;
                        tmp_path[i].h_bit = 1;
                        break;
                    }
                }

                if (repeat)
                    break;

                if (tmp_path[i].ssid > 0)
                {
                    is_wide = memcmp("WIDE", c->call, 4) == 0;
                    //is_trace = memcmp("TRACE", c->call, 5) == 0;
                    if (is_wide)
                    {
                        repeat = 1;
                        tmp_path[i].ssid--;
                        if (tmp_path[i].ssid == 0)
                            tmp_path[i].h_bit = 1;
//                        if ((is_trace) || (is_wide) && (tmp_path_size < (AX25_MAX_RPT + 2)))
                        if ((is_wide) && (tmp_path_size < (AX25_MAX_RPT + 2)))
						{
                            for (k = tmp_path_size; k > i; --k) {
                                tmp_path[k] = tmp_path[k - 1];
                            }
                            memcpy(tmp_path[i].call, APRS_Configuration.EE_MYCALL, 6);
                            tmp_path[i].ssid = APRS_Configuration.EE_MYCALL_SSID;
                            tmp_path[i].h_bit = 1;
                            tmp_path_size++;
                            i++;
                        }

                        break;
                    }
                }
            } else
						{
                if ((memcmp(tmp_path[i].call, APRS_Configuration.EE_MYCALL, 6) == 0) && (tmp_path[i].ssid == APRS_Configuration.EE_MYCALL_SSID)) {
                    repeat = 1;
                    tmp_path[i].h_bit = 1;
                    break;
                }
            }
        }

        if ((memcmp(tmp_path[1].call, APRS_Configuration.EE_MYCALL, 6) == 0) && (tmp_path[i].ssid == APRS_Configuration.EE_MYCALL_SSID))
				{
            repeat = 0;
        }

        if (repeat && is_wide)
				{
            ax25_sendVia(&ax25, tmp_path, tmp_path_size, msg->info, msg->len);
            kfile_print(&ser.fd, "REPEATED\n");
        } else {
            kfile_print(&ser.fd, "NOT REPEATED\n");
        }
    }
}

static void init(void)
{
    IRQ_ENABLE;
    kdbg_init();
    timer_init();

    ser_init(&ser, SER_UART0);
    ser_setbaudrate(&ser, 9600L);//115200L);

    eeprom_read_config();
    //print_digi_config();
  
    afsk_init(&afsk, ADC_CH, 0);
    ax25_init(&ax25, &afsk.fd, message_callback);

}

#define CMD_BUFFER_SIZE  100
    char cmd_buffer[CMD_BUFFER_SIZE] = {0};

/*
void exec_data_parse(digi_config_t *digi_conf_str)
{

}
*/

void send_config(void)
{
//int jj = 0;
    //eeprom_write_byte ((const uint8_t *)jj, 0x44);
    kfile_printf(&ser.fd,"\r\nSEND CONFIG\r\n");

//int ii=0;
//while (ii<256)
//{
    //kfile_printf(&ser.fd,"%02X ",eeprom_read_byte((const uint8_t *)ii));
    //ii++;
//}


}

void recv_config(void)
{
    kfile_printf(&ser.fd,"\r\nRECV CONFIG\r\n");    
}

void read_serial_data(void)
{
    static uint8_t buffer_position = 0;
    while(!fifo_isempty(&ser.rxfifo))
    {
        char cmd_char;
        kfile_read(&ser.fd,&cmd_char, 1);

        cmd_buffer[buffer_position]=cmd_char;
        if (buffer_position<CMD_BUFFER_SIZE-1) buffer_position++;
        kfile_printf(&ser.fd,"%c",cmd_char);

        if (cmd_char=='#')
        {
            //kfile_printf(&ser.fd,"\r\n>");
            memset(cmd_buffer, 0, sizeof(cmd_buffer));
            buffer_position = 0;
        }
        if (cmd_char=='$') 
        {
            if ((cmd_buffer[0]=='R') && (buffer_position== 2)) send_config();
            if ((cmd_buffer[0]=='S') && (buffer_position== 3)) recv_config();
            if ((cmd_buffer[0]=='P') && (buffer_position== 2)) print_digi_config();
        }            


        if (ser.rxfifo.tail==ser.rxfifo.end)
        //if (fifo_isfull(&ser.rxfifo))
        {
            //kfile_printf(&ser.fd,"\r\nBUFFER FULL \r\n");
            ser.rxfifo.head = ser.rxfifo.begin; //flush
            ser.rxfifo.tail = ser.rxfifo.head; //flush

        }

    }
}





//static AX25Call path[] = AX25_PATH(AX25_CALL(CALL_BERTOS_APRS, 0),  AX25_CALL("", 0),AX25_CALL("wide2", 1));
static AX25Call path[]     = AX25_PATH(AX25_CALL(CALL_BERTOS_APRS, 0),  AX25_CALL("", 0),AX25_CALL("wide2", 2));
bool first_boot = true;

int main(void)
{
    init();
    ticks_t start_aprs   = timer_clock();
    ticks_t start_hymtr  = timer_clock();
    ticks_t start_telem  = timer_clock();
    ticks_t start_advert = timer_clock();

    unsigned char x = 0;
    memcpy(path[1].call, APRS_Configuration.EE_MYCALL, 6);
    path[1].ssid = APRS_Configuration.EE_MYCALL_SSID;

    kfile_printf(&ser.fd, "hymDG Started\n\r");

    while (1)
    {
        ax25_poll(&ax25);
        //kfile_printf(&ser.fd,"->%d %d %d %d \r\n",ser.rxfifo.begin, ser.rxfifo.end, ser.rxfifo.head, ser.rxfifo.tail);

        if (!fifo_isempty(&ser.rxfifo)) read_serial_data();
        
#if 1
        if ((timer_clock() - start_aprs > ms_to_ticks(APRS_Configuration.EE_APRS_BEACON_INTERVAL * 1000L)) || (first_boot && (timer_clock() - start_aprs > ms_to_ticks(30 * 1000L))))
        {
            kfile_printf(&ser.fd, "Beep %d\r\n", x++);
            start_aprs = timer_clock();
            ax25_sendVia(&ax25, path, countof(path), APRS_Configuration.EE_APRS_BEACON_MSG, strlen(APRS_Configuration.EE_APRS_BEACON_MSG)-1); 
            //ax25_sendVia(&ax25, path, countof(path), APRS_Configuration.EE_APRS_BEACON_MSG, sizeof(APRS_Configuration.EE_APRS_BEACON_MSG)-1); 
                                                                                                //-1 for direwolf warning :
                                                                                                //'nul' character found in Information part.  This should never happen with APRS. 
                                                                                                //If this is meant to be APRS, TA7W-3 is transmitting with defective software.
            first_boot = false;
        }

        if ((APRS_Configuration.EE_HYMTR_BEACON_ENABLED==1) && (timer_clock() - start_hymtr) > ms_to_ticks(APRS_Configuration.EE_HYMTR_BEACON_INTERVAL * 1000L))
        {
            kfile_printf(&ser.fd, "Beep %d\r\n", x++);
            start_hymtr = timer_clock();
            ax25_sendVia(&ax25, path, countof(path), APRS_Configuration.EE_HYMTR_BEACON_MSG, strlen(APRS_Configuration.EE_HYMTR_BEACON_MSG)-1); 
            first_boot = false;
        }

        if ((APRS_Configuration.EE_TELEM_BEACON_ENABLED==1) && (timer_clock() - start_telem) > ms_to_ticks(APRS_Configuration.EE_TELEM_BEACON_INTERVAL * 1000L))
        {
            kfile_printf(&ser.fd, "Beep %d\r\n", x++);
            start_telem = timer_clock();
            ax25_sendVia(&ax25, path, countof(path), APRS_Configuration.EE_TELEM_BEACON_MSG, strlen(APRS_Configuration.EE_TELEM_BEACON_MSG)-1); 
            first_boot = false;
        }

        if ((APRS_Configuration.EE_ADVERT_BEACON_ENABLED==1) && (timer_clock() - start_advert) > ms_to_ticks(APRS_Configuration.EE_ADVERT_BEACON_INTERVAL * 1000L))
        {
            kfile_printf(&ser.fd, "Beep %d\r\n", x++);
            start_advert = timer_clock();
            ax25_sendVia(&ax25, path, countof(path), APRS_Configuration.EE_ADVERT_BEACON_MSG, strlen(APRS_Configuration.EE_ADVERT_BEACON_MSG)-1); 
            first_boot = false;
        }

#endif

    }
    return 0;
}

