#include "include.h"

#define INVALID_PARAM       0xffff

#define NEXT_PARAM_CNT      8                               //��Ŷ���:8��(2^N, ���64)
#define NEXT_PARAM_MASK     (NEXT_PARAM_CNT - 1)

#define BT_SYNC_TIME            350                         //350ms��ִ��

typedef void (*sync_callback_t)(u32 param1, u32 param2);
void tws_res_get_addr(u32 index, u32 *addr, u32 *len);
u32 bt_tws_get_waiting_res(void);

typedef union _bsp_tws_res_param_t {
    struct {
        u16 type;
        u16 num;
        u16 delay;
        u16 rsvd;
    };
    struct {
        u32 param1;
        u32 param2;
    };
} bsp_tws_res_param_t;

typedef struct _bsp_tws_sync_res_t {
    u32 tick;
    bsp_tws_res_param_t next_param[NEXT_PARAM_CNT];
    bsp_tws_res_param_t res;
    u16 res_type;
    u16 res_num;
    bool cont;
    bool play;
    s8 next_pos;
    s8 next_cnt;
} bsp_tws_sync_res_t;

bsp_tws_sync_res_t bsp_tws_sync_res;

AT(.com_text.tws_sync)
void bsp_dummy_sync(u32 param1, u32 param2)
{
}

//bt_tws_sync_run�ȴ����̵Ļص������������̫��
AT(.text.bt_tws_sync)
void bt_tws_sync_wait_cb(void)
{
    func_bt_disp_status();
}

AT(.com_text.tws_sync)
static void bsp_tws_set_sync_tick(void)
{
    bsp_tws_sync_res.tick = timer_get_tick();
}

void bsp_tws_sync_conn_callback(void)
{
    bsp_tws_sync_res.tick = timer_get_tick();
}

AT(.com_text.tws_sync)
void bsp_tws_sync_res_music(u32 param1, u32 param2)
{
    if (bsp_tws_sync_res.res_type == RES_TYPE_TONE || bsp_tws_sync_res.res_type == RES_TYPE_PIANO) {
        tone_sync_kick();
    } else if (bsp_tws_sync_res.res_type == RES_TYPE_MP3) {
        music_sync_kick();
    } else if (bsp_tws_sync_res.res_type == RES_TYPE_WAV) {
        wav_sync_kick();
    }
    if (param1 != 0) {                  //res_type != RES_TYPE_INVALID
        bsp_tws_set_sync_tick();
        bsp_tws_sync_res.res.param1 = param1;
        bsp_tws_sync_res.res.param2 = param2;
    }
}

AT(.com_rodata.tws_sync)
sync_callback_t tbl_tws_sync_hook[] = {
    bsp_dummy_sync,                         //BSP_TWS_SYNC_LED
    bsp_tws_sync_res_music,                 //BSP_TWS_SYNC_RES_MUSIC
};

void bsp_tws_sync_init(void)
{
    bt_tws_syncfunc_init(tbl_tws_sync_hook, sizeof(tbl_tws_sync_hook) / sizeof(sync_callback_t));
    memset(&bsp_tws_sync_res, 0, sizeof(bsp_tws_sync_res));
    bsp_tws_set_sync_tick();
}

//============ ͬ��LED�ӿ� ============
void bsp_tws_set_led(const void *cfg)
{
    if(bt_tws_is_connected()) {
        if(!bt_tws_is_slave()) {
            led_cal_sync_info(cfg);         //������ʾ���棬ͨ���ص�������ʾ
            bt_tws_sync_led();              //���µ�����
        } else {
            if(!bt_nor_is_connected()) {
                led_set_sta_normal(cfg);
            }
        }
    } else {
        led_set_sta_normal(cfg);
    }
}

//�ص���������������LED״̬
AT(.com_text.led_disp)
void tws_local_set_ledsta(uint8_t *buf)
{
    led_set_sync_sta(buf);
}

//�ص�������Զ������LED״̬
AT(.com_text.led_disp)
void tws_remote_set_ledsta(uint8_t *buf)
{
    if(bt_nor_is_connected()) {
        led_set_sync_sta(buf);
    }
}

//============ ͬ������ ============
//void bsp_tws_set_volume(u8 vol)
//{
//}

//============ ͬ����Դ�ӿ� ============
//������һ����Դ
void bsp_tws_sync_set_next_res(bsp_tws_res_param_t next_param)
{
    uint next_pos = (bsp_tws_sync_res.next_pos + bsp_tws_sync_res.next_cnt) & NEXT_PARAM_MASK;
    if (bsp_tws_sync_res.next_cnt > 0) {
        uint last_pos = (bsp_tws_sync_res.next_pos + bsp_tws_sync_res.next_cnt - 1) & NEXT_PARAM_MASK;
        bsp_tws_res_param_t last_param = bsp_tws_sync_res.next_param[last_pos];
        if (last_param.type == RES_TYPE_INVALID) {              //��һ����ԴΪ��Ч��Դʱ,ֱ�Ӹ���
            bsp_tws_sync_res.next_cnt--;
            next_pos = last_pos;
        }
    }
    bsp_tws_sync_res.next_param[next_pos] = next_param;
    if (bsp_tws_sync_res.next_cnt < NEXT_PARAM_CNT) {
        bsp_tws_sync_res.next_cnt++;
    } else {
        bsp_tws_sync_res.next_pos++;
    }
}

//��ȡ��һ����Դ
bsp_tws_res_param_t bsp_tws_sync_get_next_res(void)
{
    bsp_tws_res_param_t next_param;
    if (bsp_tws_sync_res.next_cnt == 0) {
        next_param.param1 = 0;
        next_param.param2 = 0;
    } else {
        uint next_pos = bsp_tws_sync_res.next_pos & NEXT_PARAM_MASK;
        next_param = bsp_tws_sync_res.next_param[next_pos];
        bsp_tws_sync_res.next_pos++;
        bsp_tws_sync_res.next_cnt--;
    }
    return next_param;
}

static void bsp_tws_music_start(void)
{
    if (!bsp_tws_sync_res.play) {
        bsp_tws_sync_res.play = true;
        bt_audio_bypass();
    }
}

static void bsp_tws_music_stop(void)
{
    if (bsp_tws_sync_res.play) {
        bsp_tws_sync_res.res_type = RES_TYPE_INVALID;               //�������,ֹͣ
        bsp_tws_sync_res.play = false;
        bsp_change_volume(sys_cb.vol);
        bt_audio_enable();
    }
}

//��ͬ����ʾ��ʱͬ������״̬
void bsp_tws_set_sta_for_res(u32 res)
{
#if BT_LOW_LATENCY_EN
    if (res == TWS_RES_MUSIC_MODE) {
        printf("music mode\n");
        bt_low_latency_disable();
    } else if (res == TWS_RES_GAME_MODE) {
        printf("game mode\n");
        bt_low_latency_enble();
    }
#endif
#if ANC_EN
    u8 anc_mode = 0;
    if (TWS_RES_NR_DISABLE == res) {
        anc_mode = 1;
    } else if (TWS_RES_ANC == res) {
        anc_mode = 2;
    } else if (TWS_RES_TRANSPARENCY == res) {
        anc_mode = 3;
    }
    if (anc_mode) {
        bsp_anc_set_mode(anc_mode - 1);

        bsp_param_write(&anc_mode, PARAM_ANC_NR_STA, 1);
        bsp_param_sync();
    }
#endif
}

//ͬ����Դ����
void bsp_tws_sync_res_process(void)
{
    u32 addr, len;
    bsp_tws_res_param_t next_param;
    bool flag_single;                                               //����״̬
    u8 music_sta = get_music_dec_sta();
	static u8 dac_sta = 0;
    if (bsp_tws_sync_res.play) {                                    //��ǰ���ڲ���״̬
        if (bsp_tws_sync_res.res_type == RES_TYPE_TONE || bsp_tws_sync_res.res_type == RES_TYPE_PIANO) {
            if (!tone_is_playing()) {
                bsp_tws_sync_res.res_type = RES_TYPE_INVALID;
                bsp_tws_sync_res.play = false;
				func_bt_set_dac(dac_sta);
                tone_play_end();
            } else {
                return;
            }
        } else if (bsp_tws_sync_res.res_type == RES_TYPE_MP3) {
            if (music_sta == MUSIC_STOP) {
                if (!bsp_tws_sync_res.cont) {
					func_bt_set_dac(dac_sta);
                    bsp_tws_music_stop();
                    bsp_tws_sync_res.res_type = 0;
                    bsp_tws_sync_res.res_num = 0;
                }
            } else if (music_sta == MUSIC_PAUSE) {
                ;
            } else {
                return;
            }
        }
    }
    if (bsp_tws_sync_res.res.type == RES_TYPE_INVALID && bsp_tws_sync_res.next_cnt == 0) {
        return;
    }
    if ((int32_t)(timer_get_tick() - bsp_tws_sync_res.tick) < bsp_tws_sync_res.res.delay) {
        return;
    }

    //printf("ring_tick: %d\n", f_bt_ring.play.ring_tick);
    //f_bt_ring.play.delta_tick = 3000;                                         //RING��ʱ

    flag_single = false;
    if (bsp_tws_sync_res.res.type != RES_TYPE_INVALID && music_sta != MUSIC_PAUSE) {
        u16 res_num = bsp_tws_sync_res.res.num;
        u16 res_type = bsp_tws_sync_res.res.type;
        bsp_tws_sync_res.res_type = res_type;
        bsp_tws_sync_res.res_num = res_num;
        if (res_type == RES_TYPE_TONE || res_type == RES_TYPE_PIANO) {
            int temp = (res_type == RES_TYPE_TONE) ? WARNING_TONE : WARNING_PIANO;
            addr = (u32)bsp_piano_get_play_res(temp, res_num);
            bsp_tws_sync_res.res.param1 = 0;
            bsp_tws_sync_res.play = true;
			dac_sta = sys_cb.dac_sta;
			func_bt_set_dac(1);
            tone_play_kick(temp, (void *)addr, !bt_tws_is_connected());
        } else if (res_type == RES_TYPE_MP3) {
            tws_res_get_addr(res_num, &addr, &len);                                 //��ȡ��Դ
            bsp_tws_music_start();

            bsp_tws_sync_res.res.param1 = 0;                                        //RES_TYPE_INVALID
            bsp_tws_sync_res.play = true;
			dac_sta = sys_cb.dac_sta;
			func_bt_set_dac(1);
            bsp_change_volume(WARNING_VOLUME);
            if (bsp_res_music_play(addr, len)) {                                    //������ͬ����Դ
                flag_single = true;
                next_param = bsp_tws_sync_get_next_res();
            }
        } else if (res_type == RES_TYPE_WAV) {
            tws_res_get_addr(res_num, &addr, &len);                                 //��ȡ��Դ
            bsp_tws_sync_res.res.param1 = 0;                                        //RES_TYPE_INVALID
            //bsp_tws_sync_res.play = true;
            dac_sta = sys_cb.dac_sta;
			func_bt_set_dac(1);
            wav_res_play(addr, len);
			func_bt_set_dac(dac_sta);
        }
        bsp_tws_set_sta_for_res(res_num);
    }
    if (!flag_single && bsp_tws_sync_res.next_cnt != 0) {
        next_param = bsp_tws_sync_get_next_res();
        bsp_tws_set_sync_tick();
        if((next_param.num >= TWS_RES_NUM_0 && next_param.num <= TWS_RES_NUM_9) || (next_param.num == TWS_RES_RING)){
        	bsp_tws_sync_res.res.delay = 500;
        }else{
            bsp_tws_sync_res.res.delay = 1000;                                        //��ֹ���͹��츲��
        }
        if (!bt_tws_set_syncfunc(BSP_TWS_SYNC_RES_MUSIC, next_param.param1, next_param.param2)) {
            flag_single = true;
        }
    }

    if (flag_single) {
        bsp_tws_set_sync_tick();
        if (!bt_tws_is_slave()) {
            bsp_tws_sync_res.res = next_param;
        }
    }
}

//ͬ������һ��MP3��Դ
void bsp_tws_res_music_play(u16 res_num)
{
    u32 addr, len;
    tws_res_get_addr(res_num, &addr, &len);
    if (addr == 0 || len == 0) {
        return;
    }
    if (!bt_tws_is_slave()) {			//��������ͬ�����ƣ���ֹ���߶��ڷ���ײ��
        bsp_tws_res_param_t res;
        res.param2 = 0;
        res.type = RES_TYPE_MP3;
        res.num = res_num;
        bsp_tws_sync_set_next_res(res);
        res.param1 = 0;
        bsp_tws_sync_set_next_res(res);
    } else {						    //���͵�����������������ͬ�����Ż���
		bt_tws_req_warning((RES_TYPE_MP3 << 8) | (res_num & 0xff));
	}

    bsp_tws_sync_res.cont = false;
}

//ͬ������һ��Tone��Դ
void bsp_tws_tone_play(u16 tone_num)
{
    if (bsp_tws_res_music_is_empty() && !tone_is_playing()) {
        if (!bt_tws_is_slave()) {
            bsp_tws_res_param_t res;
            res.param2 = 0;
            res.type = RES_TYPE_TONE;
            res.num = tone_num;
            bsp_tws_sync_set_next_res(res);
            res.param1 = 0;
            bsp_tws_sync_set_next_res(res);
        } else {		                //���͵�����������������ͬ�����Ż���
            bt_tws_req_warning((RES_TYPE_TONE << 8) | (tone_num & 0xff));
        }
    }
}

//ͬ������һ��Piano��Դ
void bsp_tws_piano_play(u16 piano_num)
{
	if (!bt_tws_is_slave()) {
        bsp_tws_res_param_t res;
        res.param2 = 0;
        res.type = RES_TYPE_PIANO;
        res.num = piano_num;
        bsp_tws_sync_set_next_res(res);
        res.param1 = 0;
        bsp_tws_sync_set_next_res(res);
	} else {	                        //���͵�����������������ͬ�����Ż���
		bt_tws_req_warning((RES_TYPE_PIANO << 8)|(piano_num & 0xff));

	}
}

//ͬ������һ����Դ
void bsp_tws_res_wav_play(u16 res_num)
{
    u32 addr, len;
    tws_res_get_addr(res_num, &addr, &len);
    if (addr == 0 || len == 0) {
        return;
    }
    if (!bt_tws_is_slave()) {
        bsp_tws_res_param_t res;
        res.param2 = 0;
        res.type = RES_TYPE_WAV;
        res.num = res_num;
        bsp_tws_sync_set_next_res(res);
        res.param1 = 0;
        bsp_tws_sync_set_next_res(res);
    } else {	                        //���͵�����������������ͬ�����Ż���
		bt_tws_req_warning((RES_TYPE_WAV << 8)|(res_num & 0xff));
	}
}

//ͬ������һ����Դ, contΪ����ģʽ���ڱ���, delayΪ���Ӽ��ʱ��
void bsp_tws_res_music_play_cont(u16 res_num, u16 delay)
{
    if (!bt_tws_is_slave()) {
        bsp_tws_res_param_t res;
        res.param2 = 0;
        res.type = RES_TYPE_MP3;
        res.num = res_num;
        res.delay = delay;
        bsp_tws_sync_set_next_res(res);
    }
    bsp_tws_sync_res.cont = true;
}

void bsp_tws_res_slave_music_play(u8 type, u8 res_num)
{
	switch(type)
	{
		case RES_TYPE_TONE:
			bsp_tws_tone_play(res_num);
			break;
		case RES_TYPE_PIANO:
			bsp_tws_piano_play(res_num);

			break;
		case RES_TYPE_MP3:
			bsp_tws_res_music_play(res_num);

			break;
		case RES_TYPE_WAV:
			bsp_tws_res_wav_play(res_num);
			break;
		default:
			break;
	}
}


//����Ƿ����������һ��
bool bsp_tws_res_music_is_empty(void)
{
    return (bsp_tws_sync_res.next_cnt == 0);
}

AT(.com_text.tws_sync)
bool bsp_tws_sync_is_play(void)
{
	return bsp_tws_sync_res.play;
}

void os_msc_sem_post(void);

//�ȴ�pendס�����ݲ����꣬��Ȼ��ϵͳ���׹ҵ�
void bsp_tws_res_music_wait_end(void)
{
    bt_tws_syncfunc_var_reset();                 //���timer����ص�����
    if(get_music_dec_sta() == MUSIC_PAUSE){      //���ڴ�����
        //printf("\n\n\n wait\n");
        os_msc_sem_post();                       //�ͷ�
    }
}

//���ͬ��������Դ
void bsp_tws_res_music_play_clear(void)
{
    bsp_tws_res_music_wait_end();
    if(get_music_dec_sta() != MUSIC_STOP) {
        music_control(MUSIC_MSG_STOP);
    }
    bsp_tws_music_stop();
    memset(&bsp_tws_sync_res, 0, sizeof(bsp_tws_sync_res));
}

#define bsp_tws_is_ring(num,type)   (bool)((type == RES_TYPE_MP3) && (((num) >= TWS_RES_NUM_0 && (num) <= TWS_RES_NUM_9) || ((num) == TWS_RES_RING)) )       //�ж��Ƿ�����

//�������δ���ŵ���ʾ��
void bsp_tws_res_incall_clear(void)
{
    bsp_tws_res_param_t next_param;

    //���׼�����Ż�ûkick��������Դ
    if (bsp_tws_is_ring(bsp_tws_sync_res.res.num,bsp_tws_sync_res.res.type)) {
        bsp_tws_sync_res.res.param1 = 0;
    }
    //���׼�������Ѿ�kick��������Դ
    if (bsp_tws_is_ring(bsp_tws_sync_res.res_num,bsp_tws_sync_res.res_type)) {
        if(get_music_dec_sta() == MUSIC_PAUSE){      //���ڴ�����
            os_msc_sem_post();                       //�ͷ�
        }
        if(get_music_dec_sta() != MUSIC_STOP) {
            music_control(MUSIC_MSG_STOP);
        }
        bsp_tws_music_stop();
    }
    //������ڵȴ���������Դ
    u32 param1 = bt_tws_get_waiting_res();
    if (bsp_tws_is_ring((param1 >> 16),(param1&0xffff))) {
        bt_tws_syncfunc_var_reset();                //���timer����ص�����
        if (bsp_tws_sync_res.next_cnt == 1) {       //�����Ӧ�Ŀ���Դ
            uint next_pos = bsp_tws_sync_res.next_pos & NEXT_PARAM_MASK;
            next_param = bsp_tws_sync_res.next_param[next_pos];
            if (next_param.param1 == 0) {
                bsp_tws_sync_res.next_cnt--;
                bsp_tws_sync_res.next_pos++;
            }
            return;
        }
    }
    //�����������һ��δ���ŵ�������Դ
    if (bsp_tws_sync_res.next_cnt != 0) {
        uint next_pos = bsp_tws_sync_res.next_pos & NEXT_PARAM_MASK;
        next_param = bsp_tws_sync_res.next_param[next_pos];
        if(bsp_tws_is_ring(next_param.num,next_param.type)) {
            next_param.param1 = 0;
            bsp_tws_sync_res.next_cnt--;
            bsp_tws_sync_res.next_pos++;
        }
    }
}

void bsp_tws_tone_play_maxvol(void)
{
    if (bsp_tws_sync_res.cont) {
        return;
    }
    //if (bsp_tws_res_music_is_empty() && !tone_is_playing()) {
        bsp_tws_tone_play(TONE_MAX_VOL);
    //}
}

//============ ͬ���ӿ� ============

//ͬ������
void bsp_tws_sync_process(void)
{
    bsp_tws_sync_res_process();
    //bsp_tws_sync_tone_process();
}
