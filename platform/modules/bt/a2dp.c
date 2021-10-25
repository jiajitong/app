#include "include.h"
#include "api.h"


#if !BT_A2DP_RECON_EN
void a2dp_recon_hook(void)
{
}
#endif

uint a2dp_get_vol(void)
{
    uint vol;

    vol = ((u32)sys_cb.vol * 1280L /VOL_MAX) /10;

    //32段音量时，同步手机音量由98%->100%调整为97%->100%(98%->100%有时会出现同步不了的情况:耳机音量32了，但手机音量97%)
    if(VOL_MAX==32 && vol==0x7C) {
        vol=0x7B;
    }

    if(vol > 0x7f) {
        vol = 0x7f;
    }

    return vol;
}

u16 a2dp_change_volume(u8 vol)
{
    uint8_t vol_set = (vol + 1) * VOL_MAX / 128;
    sys_cb.vol = vol_set;
	msg_enqueue(EVT_A2DP_VOL_SAVE);
    return bsp_volume_convert(vol_set);
}

AT(.com_text.a2dp)
void a2dp_tws_sync_vol(uint16_t vol)
{
    if (bsp_tws_sync_is_play()) {
        msg_enqueue(EVT_A2DP_SET_VOL);
    } else {
        #if !SYS_ADJ_DIGVOL_EN
        dac_set_volume(vol);
        #else
        dac_set_dvol(vol);
        #endif
    }
}
