#include "include.h"
#include "func.h"
#include "func_bt.h"

static bt_voice_cfg_t bt_voice_cfg AT(.sco_data);
static u8 bt_sys_clk AT(.sco_data);
#if BT_SNDP_DMIC_EN
static dmic_cb_t dmic_cb AT(.sco_data);
u8 cfg_micr_phase_en;
#endif
void bt_sco_rec_exit(void);
bool mic_eq_get_status(void);
void near_ains2_init(u32 nt, u8 fre_mid_min, u8 fre_high_min);
void trumpet_denoise_init(u8 level);

AT(.bt_voice.gain.tbl)
const int mic_gain_tbl[15] = {
    SOFT_DIG_P1DB,
    SOFT_DIG_P2DB,
    SOFT_DIG_P3DB,
    SOFT_DIG_P4DB,
    SOFT_DIG_P5DB,
    SOFT_DIG_P6DB,
    SOFT_DIG_P7DB,
    SOFT_DIG_P8DB,
    SOFT_DIG_P9DB,
    SOFT_DIG_P10DB,
    SOFT_DIG_P11DB,
    SOFT_DIG_P12DB,
    SOFT_DIG_P13DB,
    SOFT_DIG_P14DB,
    SOFT_DIG_P15DB,
};

////库调用，设置MIC的增益（算法之后）
AT(.bt_voice.aec)
int sco_set_mic_gain_after_aec(void)
{
    if (xcfg_cb.mic_post_gain) {
        return mic_gain_tbl[xcfg_cb.mic_post_gain - 1];
    }
    return 0;
}

ALIGNED(64)
void sco_set_incall_flag(u8 bit)
{
    GLOBAL_INT_DISABLE();
    sys_cb.incall_flag |= bit;
    GLOBAL_INT_RESTORE();
}

ALIGNED(64)
bool sco_clr_incall_flag(u8 bit)
{
    bool ret = false;
    GLOBAL_INT_DISABLE();
    if(sys_cb.incall_flag == INCALL_FLAG_FADE) {
        ret = true;
    }
    sys_cb.incall_flag &= ~bit;
    GLOBAL_INT_RESTORE();
    return ret;
}

#if BT_SCO_DBG_EN
extern const u8 btmic_ch_tbl[];
void sdadc_set_digital_gain(u8 gain);
void sco_audio_set_param(u8 type, u16 value)
{
    aec_cb_t *aec = &bt_voice_cfg.aec;
    nr_cb_t *nr = &bt_voice_cfg.nr;
    //printf("set param[%d]:%d\n", type, value);
    if (type == AEC_PARAM_NOISE) {
        xcfg_cb.bt_noise_threshoid = value;
        nr->threshoid = value;
    } else if (type == AEC_PARAM_LEVEL) {
        xcfg_cb.bt_echo_level = value;
        aec->echo_level = value;
    } else if (type == AEC_PARAM_OFFSET) {
        xcfg_cb.bt_far_offset = value;
       aec->far_offset = value;
    } else if (type == AEC_PARAM_MIC_ANL_GAIN) {
        bt_mic_anl_gain = value;
        set_mic_analog_gain(value, btmic_ch_tbl[bt_mic_sel >> 1]);
    } else if (type == AEC_PARAM_MIC_DIG_GAIN) {
        xcfg_cb.bt_dig_gain = value;
        sdadc_set_digital_gain(value);
    } else if (type == AEC_PARAM_MIC_POST_GAIN) {
        xcfg_cb.mic_post_gain = value & 0x0f;
    }
}
#endif // BT_SCO_DBG_EN

#if FUNC_BT_EN

AT(.bt_voice.ans)
void sco_ns_process(s16 *data)
{
#if BT_SNDP_EN
	bt_sndp_process(data);
#elif BT_SCO_AINS2_EN
       //ains2_process(data);           //64点
    bt_ains2_plus_process(data);        //128点
#elif BT_SCO_BCNS_EN
    bt_bcns_plus_process(data);
#elif BT_SCO_DMNS_EN
    bt_dmns_plus_process(data);
#elif BT_SNDP_DMIC_EN && BT_NLMS_AEC_EN
    bt_sndp_dm_process(data);
#endif
}

void sco_audio_init(void)
{
    u8 sysclk;
    memset(&bt_voice_cfg, 0, sizeof(bt_voice_cfg_t));
#if BT_AEC_EN || BT_ALC_EN || BT_NLMS_AEC_EN
    aec_cb_t *aec = &bt_voice_cfg.aec;
#endif
    nr_cb_t *nr = &bt_voice_cfg.nr;

#if FOT_EN
    fot_update_pause();
#endif

#if BT_AEC_EN || BT_ALC_EN || BT_NLMS_AEC_EN
    if (xcfg_cb.bt_aec_en) {
        aec_init_dft(aec);
    #if BT_AEC_EN
        aec->aec_en = 1;
        aec->rfu[0] = 0;       //0:自动模式 1:手动模式
        aec->echo_level = BT_ECHO_LEVEL;
        aec->far_pass_nr = xcfg_cb.bt_aec_far_nr_en;
    #elif BT_NLMS_AEC_EN
        aec->nlms_aec_en = 1;
    #endif
        aec->far_offset = BT_FAR_OFFSET;

    } else if (xcfg_cb.bt_alc_en) {
#if BT_ALC_EN
        alc_cb_t *alc = &bt_voice_cfg.alc;
        alc_init_dft(alc);
        alc->alc_en = 1;
        alc->fade_in_delay = BT_ALC_FADE_IN_DELAY;
        alc->fade_in_step = BT_ALC_FADE_IN_STEP;
        alc->fade_out_delay = BT_ALC_FADE_OUT_DELAY;
        alc->fade_out_step = BT_ALC_FADE_OUT_STEP;
        alc->far_voice_thr = BT_ALC_VOICE_THR;
#endif
    }
#endif

#if TINY_TRANSPARENCY_EN
    bsp_ttp_stop();
#endif

    sysclk = get_cur_sysclk();
    if (sys_cb.wav_sysclk_bak == 0xff) {
        bt_sys_clk = sysclk;
    } else {
        bt_sys_clk = sys_cb.wav_sysclk_bak;
    }

    nr->type = NR_CFG_TYPE_DEFUALT;
    nr->mode = NR_CFG_MODE0;
    nr->level = 18;
    nr->threshoid = BT_NOISE_THRESHOID;

#if BT_SNDP_DMIC_EN
    if (xcfg_cb.bt_sndp_dm_en && xcfg_cb.micl_en && xcfg_cb.micr_en) {
        nr->dmic = &dmic_cb;
        memset(&dmic_cb, 0, sizeof(dmic_cb_t));
        dmic_init_dft(&dmic_cb);
        dmic_cb.mic_cfg      = (xcfg_cb.micl_cfg == 1)? 0 : BIT(0);
        dmic_cb.distance     = xcfg_cb.bt_sndp_dm_distance;         //23mm(默认)
        dmic_cb.degree       = xcfg_cb.bt_sndp_dm_degree;           //30度(默认)
        dmic_cb.max_degree   = xcfg_cb.bt_sndp_dm_max_degree;       //90度
        dmic_cb.level        = xcfg_cb.bt_sndp_dm_level;            //30dB(默认)
        dmic_cb.wind_level   = xcfg_cb.bt_sndp_dm_wind_level;       //风噪降噪量（默认20dB）
        dmic_cb.snr_x0_level = xcfg_cb.bt_sndp_dm_snr_x0_level;     //如果在一般信噪比下，降噪不足，就适当调大，但不要大于8
        dmic_cb.snr_x1_level = xcfg_cb.bt_sndp_dm_snr_x1_level;     //如果低信噪比下，语音断断续续，就适当调小，但不要低于10
        //默认关闭以下参数，需要可手动打开
//		dmic_cb.rfu[0]       = 0;									//是否开启双麦参数打印
//        dmic_cb.maxIR        = 131072;
//        dmic_cb.maxIR2       = 91750;
//        dmic_cb.windnoise_conditioned_eq_x0 = 80;                   //80(2500Hz@16kHz, 默认),  风噪抑制起始频段(Hz)
//        dmic_cb.windnoise_conditioned_eq_x1 = 208;                  //208(6500Hz@16kH, 默认),  风噪抑制终止频段(Hz)
//        dmic_cb.windnoise_conditioned_eq_y1 = 1677722;              //1677722(-20dB, 1<<24, 默认), 风噪抑制的终止大小(db), 风噪抑制的起始大小为0db
        #ifdef RES_BUF_DMIC_SNDP_BLOCK_BIN
        if(0 != RES_LEN_DMIC_SNDP_BLOCK_BIN){
            dmic_cb.filter_coef = (int*)RES_BUF_DMIC_SNDP_BLOCK_BIN;
            dmic_cb.maxIR       = xcfg_cb.bt_sndp_dm_maxir;
        }
        #endif // RES_BUF_DMIC_SNDP_BLOCK_BIN
        nr->type = NR_CFG_TYPE_DMIC;
    #if BT_NLMS_AEC_EN
        sysclk = sysclk < SYS_160M ? SYS_160M : sysclk; //SYS_147M SYS_160M
    #else
        sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
    #endif
    }
    if(xcfg_cb.micr_phase_en == 1){
        cfg_micr_phase_en = 1;
    }
#if BT_TRUMPET_DENOISE_EN
    trumpet_denoise_init(xcfg_cb.bt_trumpet_level);
    nr->trumpet_denoise_en = 1;
#endif
#endif

#if BT_SCO_FAR_NR_EN
    nr->type |= NR_CFG_TYPE_FAR;
#if BT_SCO_FAR_NR_SELECT
    nr->far_nr_type = 1;
    far_ains2_init(BT_SCO_FAR_NOISE_LEVEL,0,0);

#if BT_NLMS_AEC_EN
    sysclk = sysclk < SYS_160M ? SYS_160M : sysclk;
#else
    sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
#endif

#else
    nr->far_nr_type = 0;
    nr_init(((u32)BT_SCO_FAR_NOISE_LEVEL << 16) | BT_SCO_FAR_NOISE_THRESHOID);

#if BT_NLMS_AEC_EN
    sysclk = sysclk < SYS_147M ? SYS_147M : sysclk;
#else
    sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
#endif

#endif

#endif

#if BT_SCO_CALLING_NR_EN
    nr->calling_voice_cnt = BT_SCO_CALLING_VOICE_CNT;
    nr->calling_voice_pow = BT_SCO_CALLING_VOICE_POW;
#endif // BT_SCO_CALLING_NR_EN

#if BT_SCO_ANS_EN
    nr->type = (nr->type & ~0x07) | NR_CFG_TYPE_ANS;
#endif

#if BT_SCO_BCNS_EN
	nr->type = (nr->type & ~0x07) | NR_CFG_TYPE_BCNS;
	nr->mode = BT_SCO_BCNS_MODE;
	bcns_init(BT_SCO_BCNS_LEVEL, BT_BCNS_VAD_VALUE, BT_BCNS_ENLARGE);
	sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
#endif

#if BT_SCO_DMNS_EN
	nr->type = (nr->type & ~0x07) | NR_CFG_TYPE_DMNS;
	nr->mode = BT_SCO_DMNS_MODE;
	dmns_init(BT_SCO_DMNS_LEVEL);
	sysclk = sysclk < SYS_80M ? SYS_80M : sysclk;
#endif

#if BT_SCO_AINS2_EN
	nr->type = (nr->type & ~0x07) | NR_CFG_TYPE_AINS2;
	nr->mode = BT_SCO_AINS2_MODE;
	ains2_init(BT_SCO_AINS2_LEVEL,BT_FRE_MID_MIN,BT_FRE_HIGH_MIN);
	sysclk = sysclk < SYS_80M ? SYS_80M : sysclk;
#endif

#if BT_SNDP_EN
    nr->type = (nr->type & ~0x07) | NR_CFG_TYPE_SNDP;
	nr->mode = BT_SNDP_MODE;
    nr->level = xcfg_cb.bt_sndp_level;                      //20dB(默认)

#if BT_NEAR_AINS2_EN
    nr->near_ains2_en = 1;
    near_ains2_init(BT_NEAR_AINS2_NOISE_LEVEL,0,0);
    #if BT_NLMS_AEC_EN
        sysclk = sysclk < SYS_147M ? SYS_147M : sysclk;
    #else
        sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
    #endif
#endif

#if BT_NLMS_AEC_EN
    sysclk = sysclk < SYS_80M ? SYS_80M : sysclk;
#else
    sysclk = sysclk < SYS_60M ? SYS_60M : sysclk;
#endif

#if BT_TRUMPET_DENOISE_EN
    trumpet_denoise_init(xcfg_cb.bt_trumpet_level);
    nr->trumpet_denoise_en = 1;
#endif
#endif

    if (xcfg_cb.huart_en) {
    #if BT_SNDP_DUMP_EN || BT_AINS2_DUMP_EN
        nr->dump_en = BIT(0)|BIT(1)|BIT(2);
    #elif BT_AEC_DUMP_EN
        nr->dump_en = BIT(3)|BIT(4)| BIT(5);
    #elif BT_SCO_FAR_DUMP_EN
        nr->dump_en = BIT(4)| BIT(5);
    #endif
        sysclk = sysclk < SYS_120M ? SYS_120M : sysclk;
    }

    if (sys_cb.wav_sysclk_bak != 0xff) {
        sys_cb.wav_sysclk_bak = sysclk;
        sysclk = SYS_120M;
    }
    set_sys_clk(sysclk);

#if BT_HFP_REC_EN
    func_bt_sco_rec_init();
#endif
#if BT_MIC_DRC_EN
    u8 *mic_drc_addr;
    u32 mic_drc_len;
#endif
    u32 mic_eq_addr, mic_eq_len;
    if (bt_sco_is_msbc()) {
        mic_eq_addr = RES_BUF_EQ_BT_MIC_16K_EQ;
        mic_eq_len  = RES_LEN_EQ_BT_MIC_16K_EQ;
    #if BT_MIC_DRC_EN
        mic_drc_addr = (u8 *)RES_BUF_EQ_BT_MIC_16K_DRC;
        mic_drc_len = RES_LEN_EQ_BT_MIC_16K_DRC;
    #endif
    } else {
        mic_eq_addr = RES_BUF_EQ_BT_MIC_8K_EQ;
        mic_eq_len  = RES_LEN_EQ_BT_MIC_8K_EQ;
    #if BT_MIC_DRC_EN
        mic_drc_addr = (u8 *)RES_BUF_EQ_BT_MIC_8K_DRC;
        mic_drc_len = RES_LEN_EQ_BT_MIC_8K_DRC;
    #endif
    }
#ifdef RES_BUF_EQ_CALL_NORMAL_EQ
    music_set_eq_by_res(RES_BUF_EQ_CALL_NORMAL_EQ, RES_LEN_EQ_CALL_NORMAL_EQ);
#else
    music_eq_off();
#endif
#if DMIC_DBG_EN || SMIC_DBG_EN
    if(!mic_eq_get_status())
#endif
    {
        if (mic_set_eq_by_res(mic_eq_addr, mic_eq_len)) {
            bt_voice_cfg.mic_eq_en = 1;
        }
    #if BT_MIC_DRC_EN
        if (mic_drc_init(mic_drc_addr, mic_drc_len)) {
            bt_voice_cfg.mic_drc_en = 1;
        }
    #endif
    }

    sco_set_incall_flag(INCALL_FLAG_SCO);
#if SYS_KARAOK_EN
    bsp_karaok_exit(AUDIO_PATH_KARAOK);
    sys_cb.hfp_karaok_en = BT_HFP_CALL_KARAOK_EN;       //通话是否支持KARAOK
    plugin_hfp_karaok_configure();
    if (sys_cb.hfp_karaok_en) {
        bsp_karaok_init(AUDIO_PATH_BTMIC, FUNC_BT);
        kara_sco_start();
    } else
#endif
    {
        bt_voice_init(&bt_voice_cfg);
        dac_set_anl_offset(1);
        #if I2S_EXT_EN && I2S_2_BT_SCO_EN
        if (bt_sco_is_msbc()) {                         //如果开了msbc，则采样率设为16k
            dac_spr_set(SPR_16000);
        } else {
            dac_spr_set(SPR_8000);
        }
        iis_start();
        #else
        audio_path_init(AUDIO_PATH_BTMIC);
        audio_path_start(AUDIO_PATH_BTMIC);
        #endif // I2S_EXT_EN
        bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
        dac_fade_in();
    }
}

void sco_audio_exit(void)
{
    sco_clr_incall_flag(INCALL_FLAG_SCO);
#if BT_HFP_REC_EN
    sfunc_record_stop();
    bt_sco_rec_exit();
#endif
#if SYS_KARAOK_EN
    if (sys_cb.hfp_karaok_en) {
        kara_sco_stop();
        bsp_karaok_exit(AUDIO_PATH_BTMIC);
        sys_cb.hfp_karaok_en = 0;
    } else
#endif
    {
        dac_fade_out();
        dac_aubuf_clr();
        dac_set_anl_offset(0);
        bsp_change_volume(sys_cb.vol);
        #if I2S_EXT_EN && I2S_2_BT_SCO_EN
        iis_stop();
        #endif // I2S_EXT_EN
        audio_path_exit(AUDIO_PATH_BTMIC);
        bt_voice_exit();
    }

#if SYS_KARAOK_EN
    bsp_karaok_init(AUDIO_PATH_KARAOK, FUNC_BT);
#endif

#if BT_REC_EN && BT_HFP_REC_EN
    bt_music_rec_init();
#endif

    set_sys_clk(bt_sys_clk);

#if TINY_TRANSPARENCY_EN
    bsp_anc_set_mode(sys_cb.anc_user_mode);
#endif

#if FOT_EN
    fot_update_continue();
#endif

    music_set_eq_by_num(sys_cb.eq_mode);
}

static void sfunc_bt_call_process(void)
{
    func_process();
#if BT_TWS_MS_SWITCH_EN
    if (xcfg_cb.bt_tswi_sco_en && bt_tws_need_switch(0)) {
        printf("AUDIO SWITCH\n");
        bt_tws_switch();
    }
#endif
    func_bt_sub_process();
    func_bt_status();
}

static void sfunc_bt_call_enter(void)
{
    sco_set_incall_flag(INCALL_FLAG_CALL);
    if(sys_cb.incall_flag == INCALL_FLAG_FADE) {
        bsp_change_volume(bsp_bt_get_hfp_vol(sys_cb.hfp_vol));
        dac_fade_in();
    }
#if DAC_DNR_EN
    dac_dnr_set_sta(0);
#endif
}

static void sfunc_bt_call_exit(void)
{
#if DAC_DNR_EN
    if (!bt_is_low_latency()) {
        dac_dnr_set_sta(sys_cb.dnr_sta);
    }
#endif
    bool vol_change = sco_clr_incall_flag(INCALL_FLAG_CALL);
    if(vol_change) {
        bsp_change_volume(sys_cb.vol);
    }
}

AT(.text.func.bt)
void sfunc_bt_call(void)
{
    printf("%s\n", __func__);

    sfunc_bt_call_enter();

    while ((f_bt.disp_status >= BT_STA_OUTGOING) && (func_cb.sta == FUNC_BT)) {
        sfunc_bt_call_process();
        sfunc_bt_call_message(msg_dequeue());
        func_bt_display();
    }
    sfunc_bt_call_exit();
}
#else

void sco_audio_init(void){}
void sco_audio_exit(void){}

#endif //FUNC_BT_EN
