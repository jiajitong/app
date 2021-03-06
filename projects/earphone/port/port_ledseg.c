#include "include.h"

#if (GUI_SELECT == GUI_LEDSEG_7P7S)

//-------------------------------------------------------------
#define LED7P7S_ANY_IO          0                                   //Support Any IO drv ledseg7p7s

#define LEDSEG_ALL_DIRIN() {GPIOADE |=0x7F; GPIOADIR |= 0x7F; }
#define LEDSEG_HDRV_EN()   {GPIOADE |=0x7F;  GPIOADRV |= 0x7F;}
#define LEDSEG_HDRV_DIS()  {GPIOADRV &= ~0x7F;}
#define LEDSEG_ALL_CLR() {LEDSEG_ALL_DIRIN(); LEDSEG_HDRV_EN();}    //DirIn, Open High Drv.
#define LEDSEG_ALL_OFF() {LEDSEG_ALL_DIRIN(); LEDSEG_HDRV_DIS();}   //DirIn, Close High Drv.

//note, must first IO "SET", then set "DIR". Otherwise have some unwanted pulse
#define LEDSEG0_H()  {GPIOASET = BIT(0); GPIOADIR &= ~BIT(0);}
#define LEDSEG0_L()  {GPIOACLR = BIT(0); GPIOADIR &= ~BIT(0);}

#define LEDSEG1_H()  {GPIOASET = BIT(1); GPIOADIR &= ~BIT(1);}
#define LEDSEG1_L()  {GPIOACLR = BIT(1); GPIOADIR &= ~BIT(1);}

#define LEDSEG2_H()  {GPIOASET = BIT(2); GPIOADIR &= ~BIT(2);}
#define LEDSEG2_L()  {GPIOACLR = BIT(2); GPIOADIR &= ~BIT(2);}

#define LEDSEG3_H()  {GPIOASET = BIT(3); GPIOADIR &= ~BIT(3);}
#define LEDSEG3_L()  {GPIOACLR = BIT(3); GPIOADIR &= ~BIT(3);}

#define LEDSEG4_H()  {GPIOASET = BIT(4); GPIOADIR &= ~BIT(4);}
#define LEDSEG4_L()  {GPIOACLR = BIT(4); GPIOADIR &= ~BIT(4);}

#define LEDSEG5_H()  {GPIOASET = BIT(5); GPIOADIR &= ~BIT(5);}
#define LEDSEG5_L()  {GPIOACLR = BIT(5); GPIOADIR &= ~BIT(5);}

#define LEDSEG6_H()  {GPIOASET = BIT(6); GPIOADIR &= ~BIT(6);}
#define LEDSEG6_L()  {GPIOACLR = BIT(6); GPIOADIR &= ~BIT(6);}

typedef void (*PFUNC) (void);

AT(.com_text.ledseg)
static void led_comm_pin0(void)
{
    LEDSEG0_H();
}

AT(.com_text.ledseg)
static void led_comm_pin1(void)
{
    LEDSEG1_H();
}

AT(.com_text.ledseg)
static void led_comm_pin2(void)
{
    LEDSEG2_H();
}

AT(.com_text.ledseg)
static void led_comm_pin3(void)
{
    LEDSEG3_H();
}

AT(.com_text.ledseg)
static void led_comm_pin4(void)
{
    LEDSEG4_H();
}

AT(.com_text.ledseg)
static void led_comm_pin5(void)
{
    LEDSEG5_H();
}

AT(.com_text.ledseg)
static void led_comm_pin6(void)
{
    LEDSEG6_H();
}

AT(.com_text.ledseg.table)
const PFUNC ledcomm_pfunc[] = {
    led_comm_pin0,
    led_comm_pin1,
    led_comm_pin2,
    led_comm_pin3,
    led_comm_pin4,
    led_comm_pin5,
    led_comm_pin6,
};

//-------------------------------------------------------------
AT(.com_text.ledseg)
void ledseg_7p7s_set(uint8_t seg_bits, uint8_t com_pin)
{
#if LED7P7S_ANY_IO
    void (*pfunc)(void);

    if( 0 == seg_bits){
        return;
    }
    LEDSEG_ALL_DIRIN();      //IO???????????????????????????

    //seg_bits display
    if (seg_bits & BIT(0)) {
        LEDSEG0_L();
    }
    if (seg_bits & BIT(1)) {
        LEDSEG1_L();
    }
    if (seg_bits & BIT(2)) {
        LEDSEG2_L();
    }
    if (seg_bits & BIT(3)) {
        LEDSEG3_L();
    }
    if (seg_bits & BIT(4)) {
        LEDSEG4_L();
    }
    if (seg_bits & BIT(5)) {
        LEDSEG5_L();
    }
    if (seg_bits & BIT(6)) {
        LEDSEG6_L();
    }

    //com high in sequence
    pfunc = ledcomm_pfunc[com_pin];
    (*pfunc)();
#else   //PA0~PA6
    u8 gpio_out = GPIOA & 0x80;
    u8 gpio_dir = GPIOADIR | 0x7f;
    if (seg_bits) {
        gpio_dir &= ~seg_bits;                  //???????????????SEG???????????????????????????
        gpio_dir &= ~BIT(com_pin);              //??????COM?????????????????????
        gpio_out |= BIT(com_pin);
    }
    GPIOA = gpio_out;
    GPIOADIR = gpio_dir;
#endif
}


AT(.com_text.ledseg)
void ledseg_7p7s_clr(void)
{
#if LED7P7S_ANY_IO
    LEDSEG_ALL_CLR();
#else
    GPIOADE |= 0x7f;
    GPIOADIR |= 0x7f;                       //???????????????????????????
    GPIOADRV |= 0x7f;                       //32mA????????????
#endif
}

AT(.text.ledseg)
void ledseg_7p7s_off(void)
{
    ledseg_7p7s_init();
#if LED7P7S_ANY_IO
    LEDSEG_ALL_OFF();
#else
    GPIOADE |= 0x7f;
    GPIOADIR |= 0x7f;
    GPIOADRV &= ~0x7f;
#endif
}

AT(.com_text.ledseg)
void ledseg_7p7s_scan_hook(u8 *buf)
{
}

AT(.com_text.ledseg)
bool ledseg_7p7s_reuse_hook(void)
{
    return false;
}

//??????7???????????????:
//  |--A--|
//  F     B
//  |--G--|
//  E     C
//  |--D--|

AT(.text.ledseg)
void ledseg_7p7s_update_dispbuf(void)
{
    u8 dis_buf[7];
    memset(dis_buf, 0, 7);

    if (ledseg_buf[0] & SEG_A)  dis_buf[0] |= PIN1;
    if (ledseg_buf[0] & SEG_B)  dis_buf[0] |= PIN2;
    if (ledseg_buf[0] & SEG_C)  dis_buf[3] |= PIN0;
    if (ledseg_buf[0] & SEG_D)  dis_buf[4] |= PIN0;
    if (ledseg_buf[0] & SEG_E)  dis_buf[0] |= PIN3;
    if (ledseg_buf[0] & SEG_F)  dis_buf[1] |= PIN0;
    if (ledseg_buf[0] & SEG_G)  dis_buf[2] |= PIN0;

    if (ledseg_buf[1] & SEG_A)  dis_buf[1] |= PIN2;
    if (ledseg_buf[1] & SEG_B)  dis_buf[1] |= PIN3;
    if (ledseg_buf[1] & SEG_C)  dis_buf[4] |= PIN1;
    if (ledseg_buf[1] & SEG_D)  dis_buf[1] |= PIN5;
    if (ledseg_buf[1] & SEG_E)  dis_buf[1] |= PIN4;
    if (ledseg_buf[1] & SEG_F)  dis_buf[2] |= PIN1;
    if (ledseg_buf[1] & SEG_G)  dis_buf[3] |= PIN1;

    if (ledseg_buf[2] & SEG_A)  dis_buf[4] |= PIN3;
    if (ledseg_buf[2] & SEG_B)  dis_buf[2] |= PIN4;
    if (ledseg_buf[2] & SEG_C)  dis_buf[3] |= PIN4;
    if (ledseg_buf[2] & SEG_D)  dis_buf[5] |= PIN0;
    if (ledseg_buf[2] & SEG_E)  dis_buf[5] |= PIN2;
    if (ledseg_buf[2] & SEG_F)  dis_buf[3] |= PIN2;
    if (ledseg_buf[2] & SEG_G)  dis_buf[4] |= PIN2;

    if (ledseg_buf[3] & SEG_A)  dis_buf[6] |= PIN5;
    if (ledseg_buf[3] & SEG_B)  dis_buf[5] |= PIN6;
    if (ledseg_buf[3] & SEG_C)  dis_buf[4] |= PIN5;
    if (ledseg_buf[3] & SEG_D)  dis_buf[5] |= PIN3;
    if (ledseg_buf[3] & SEG_E)  dis_buf[3] |= PIN5;
    if (ledseg_buf[3] & SEG_F)  dis_buf[5] |= PIN4;
    if (ledseg_buf[3] & SEG_G)  dis_buf[4] |= PIN6;

    if (ledseg_buf[4] & ICON_PLAY)   dis_buf[0] |= PIN5;
    if (ledseg_buf[4] & ICON_PAUSE)  dis_buf[2] |= PIN5;
    if (ledseg_buf[4] & ICON_USB)    dis_buf[5] |= PIN1;
    if (ledseg_buf[4] & ICON_SD)     dis_buf[0] |= PIN4;
    if (ledseg_buf[4] & ICON_DDOT)   dis_buf[2] |= PIN3;
    if (ledseg_buf[4] & ICON_FM)     dis_buf[6] |= PIN2;
    if (ledseg_buf[4] & ICON_MP3)    dis_buf[2] |= PIN6;

    ledseg_7p7s_update_dispbuf_do(dis_buf);
}
#elif (GUI_SELECT == GUI_LEDSEG_5P7S)

//-------------------------------------------------------------

#define LEDSEG_ALL_DIRIN() {GPIOBDE |= BIT(2); GPIOBDIR |= BIT(2); GPIOEDE |= 0xF0; GPIOEDIR |= 0xF0; }
#define LEDSEG_HDRV_EN()   {GPIOBDE |= BIT(2);  GPIOBDRV |= BIT(2); GPIOEDE |= 0xF0;  GPIOEDRV |= 0xF0; }
#define LEDSEG_HDRV_DIS()  {GPIOBDRV &= ~BIT(2); GPIOEDRV &= ~0xF0; }
#define LEDSEG_ALL_CLR() {LEDSEG_ALL_DIRIN(); LEDSEG_HDRV_EN();}    //DirIn, Open High Drv.
#define LEDSEG_ALL_OFF() {LEDSEG_ALL_DIRIN(); LEDSEG_HDRV_DIS();}   //DirIn, Close High Drv.

//note, must first IO "SET", then set "DIR". Otherwise have some unwanted pulse
#define LEDSEG0_H()  {GPIOBSET = BIT(2); GPIOBDIR &= ~BIT(2);}
#define LEDSEG0_L()  {GPIOBCLR = BIT(2); GPIOBDIR &= ~BIT(2);}

#define LEDSEG1_H()  {GPIOESET = BIT(5); GPIOEDIR &= ~BIT(5);}
#define LEDSEG1_L()  {GPIOECLR = BIT(5); GPIOEDIR &= ~BIT(5);}

#define LEDSEG2_H()  {GPIOESET = BIT(6); GPIOEDIR &= ~BIT(6);}
#define LEDSEG2_L()  {GPIOECLR = BIT(6); GPIOEDIR &= ~BIT(6);}

#define LEDSEG3_H()  {GPIOESET = BIT(7); GPIOEDIR &= ~BIT(7);}
#define LEDSEG3_L()  {GPIOECLR = BIT(7); GPIOEDIR &= ~BIT(7);}

#define LEDSEG4_H()  {GPIOESET = BIT(4); GPIOEDIR &= ~BIT(4);}
#define LEDSEG4_L()  {GPIOECLR = BIT(4); GPIOEDIR &= ~BIT(4);}

typedef void (*PFUNC) (void);

AT(.com_text.ledseg)
static void led_comm_pin0(void)
{
    LEDSEG0_H();
}

AT(.com_text.ledseg)
static void led_comm_pin1(void)
{
    LEDSEG1_H();
}

AT(.com_text.ledseg)
static void led_comm_pin2(void)
{
    LEDSEG2_H();
}

AT(.com_text.ledseg)
static void led_comm_pin3(void)
{
    LEDSEG3_H();
}

AT(.com_text.ledseg)
static void led_comm_pin4(void)
{
    LEDSEG4_H();
}

AT(.com_text.ledseg.table)
const PFUNC ledcomm_pfunc[] = {
    led_comm_pin0,
    led_comm_pin1,
    led_comm_pin2,
    led_comm_pin3,
    led_comm_pin4,
};

//-------------------------------------------------------------
AT(.com_text.ledseg)
void ledseg_5p7s_set(uint8_t seg_bits, uint8_t com_pin)
{
    void (*pfunc)(void);

    if( 0 == seg_bits){
        return;
    }
    LEDSEG_ALL_DIRIN();      //IO???????????????????????????

    //seg_bits display
    if (seg_bits & BIT(0)) {
        LEDSEG0_L();
    }
    if (seg_bits & BIT(1)) {
        LEDSEG1_L();
    }
    if (seg_bits & BIT(2)) {
        LEDSEG2_L();
    }
    if (seg_bits & BIT(3)) {
        LEDSEG3_L();
    }
    if (seg_bits & BIT(4)) {
        LEDSEG4_L();
    }

    //com high in sequence
    pfunc = ledcomm_pfunc[com_pin];
    (*pfunc)();
}

AT(.com_text.ledseg)
void ledseg_5p7s_clr(void)
{
    LEDSEG_ALL_CLR();
}

AT(.text.ledseg)
void ledseg_5p7s_off(void)
{
    ledseg_5p7s_init();
    LEDSEG_ALL_OFF();
}

AT(.com_text.ledseg)
void ledseg_5p7s_scan_hook(u8 *buf)
{
}

AT(.com_text.ledseg)
bool ledseg_5p7s_reuse_hook(void)
{
    return false;
}

//??????7???????????????:
//  |--A--|
//  F     B
//  |--G--|
//  E     C
//  |--D--|

AT(.text.ledseg)
void ledseg_5p7s_update_dispbuf(void)
{
    u8 dis_buf[5];
    memset(dis_buf, 0, 5);

    if (ledseg_buf[0] & SEG_B)  dis_buf[2] |= PIN3;
    if (ledseg_buf[0] & SEG_C)  dis_buf[1] |= PIN3;

    if (ledseg_buf[1] & SEG_A)  dis_buf[1] |= PIN2;
    if (ledseg_buf[1] & SEG_B)  dis_buf[2] |= PIN1;
    if (ledseg_buf[1] & SEG_C)  dis_buf[3] |= PIN2;
    if (ledseg_buf[1] & SEG_D)  dis_buf[3] |= PIN1;
    if (ledseg_buf[1] & SEG_E)  dis_buf[4] |= PIN1;
    if (ledseg_buf[1] & SEG_F)  dis_buf[4] |= PIN2;
    if (ledseg_buf[1] & SEG_G)  dis_buf[4] |= PIN3;

    if (ledseg_buf[2] & SEG_A)  dis_buf[0] |= PIN1;
    if (ledseg_buf[2] & SEG_B)  dis_buf[1] |= PIN0;
    if (ledseg_buf[2] & SEG_C)  dis_buf[0] |= PIN2;
    if (ledseg_buf[2] & SEG_D)  dis_buf[2] |= PIN0;
    if (ledseg_buf[2] & SEG_E)  dis_buf[0] |= PIN3;
    if (ledseg_buf[2] & SEG_F)  dis_buf[3] |= PIN0;
    if (ledseg_buf[2] & SEG_G)  dis_buf[4] |= PIN0;

    ledseg_5p7s_update_dispbuf_do(dis_buf);
}

#endif  // #if (GUI_SELECT == GUI_LEDSEG_7P7S)
