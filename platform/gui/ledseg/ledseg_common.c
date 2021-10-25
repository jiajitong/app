#include "include.h"
#include "ledseg_common.h"
#include "ledseg_7p7s.h"
#include "ledseg_5p7s.h"

#if (GUI_SELECT & DISPLAY_LEDSEG)

u8 ledseg_buf[NUM_CNT] AT(.buf.ledseg);
u8 ledseg_disp_num AT(.buf.ledseg);

typedef void (*PFUNC)(void);
extern const PFUNC ledseg_disp_pfunc[];

AT(.rodata.ledseg)
const u8 ledseg_num_table[10] =
    {
        T_LEDSEG_0,
        T_LEDSEG_1,
        T_LEDSEG_2,
        T_LEDSEG_3,
        T_LEDSEG_4,
        T_LEDSEG_5,
        T_LEDSEG_6,
        T_LEDSEG_7,
        T_LEDSEG_8,
        T_LEDSEG_9,
};

#if (GUI_SELECT == GUI_LEDSEG_5P7S)
AT(.text.ledseg)
void ledseg_disp_number(u8 num)
{
    if (num < 10)
        ledseg_buf[2] = ledseg_num_table[num];
    else if (num < 100)
    {
        ledseg_buf[1] = ledseg_num_table[num / 10];
        ledseg_buf[2] = ledseg_num_table[num % 10];
    }
    else
    {
        ledseg_buf[0] = ledseg_num_table[1];
        ledseg_buf[1] = ledseg_num_table[0];
        ledseg_buf[2] = ledseg_num_table[0];
    }
}
#else
AT(.text.ledseg)
void ledseg_disp_number(u16 num)
{
    if (num > 9999)
    {
        num = 9999;
    }
    ledseg_buf[0] = ledseg_num_table[num / 1000];
    ledseg_buf[1] = ledseg_num_table[(num % 1000) / 100];
    ledseg_buf[2] = ledseg_num_table[(num % 100) / 10];
    ledseg_buf[3] = ledseg_num_table[num % 10];
}
#endif

AT(.text.ledseg)
void ledseg_display(u8 disp_num)
{
    u8 buf_bak[NUM_CNT];

    void (*pfunc)(void);
    memcpy(buf_bak, ledseg_buf, NUM_CNT);
    memset(ledseg_buf, 0, sizeof(ledseg_buf));
    pfunc = ledseg_disp_pfunc[disp_num];
    (*pfunc)();
    if (memcmp(buf_bak, ledseg_buf, NUM_CNT) == 0)
    {
        return;
    }
    ledseg_disp_num = disp_num;
    ledseg_update_dispbuf();
}

AT(.text.ledseg)
void ledseg_displaybat(bool sta)
{
    static u8 charge_l;
    u8 bat_level = bsp_get_bat_level();
    u16 bat_off = LPWR_OFF_VBAT * 100 + 2700;

    if (sta == IN_CHARGE)
    {
        bat_level = (sys_cb.vbat - bat_off) / ((4150 - bat_off) / 100);
        if (charge_l < bat_level)
        {
            charge_l = bat_level;
        }
        ledseg_disp_number(charge_l);
    }
    else
    {
        ledseg_disp_number(bat_level);
    }
    ledseg_update_dispbuf();
}

#endif
