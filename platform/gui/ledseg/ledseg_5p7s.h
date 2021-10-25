#ifndef _LEDSEG_5P7S_H
#define _LEDSEG_5P7S_H

/* ---------------------------------------
** | PIN0  PIN1  PIN2  PIN3  PIN4  PIN5  |
** |  |     |     |     |     |     |    |
** | PJ0   PJ1   PJ2   PJ3   PJ4   PJ5   |
** --------------------------------------- */

#define PIN0  BIT(0)
#define PIN1  BIT(1)
#define PIN2  BIT(2)
#define PIN3  BIT(3)
#define PIN4  BIT(4)

void ledseg_5p7s_init(void);
void ledseg_5p7s_update_dispbuf(void);
void ledseg_5p7s_scan(void);
void ledseg_5p7s_update_dispbuf_do(u8 *disp_buf);
void ledseg_5p7s_off(void);
void ledseg_5p7s_clr(void);
void ledseg_5p7s_scan_hook(u8 *buf);

void ledseg_wko(bool on_off);

#endif