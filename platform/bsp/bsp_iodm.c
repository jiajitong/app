#include "include.h"


#if IODM_TEST_MODE

#define TRACE_EN                0

#if TRACE_EN
#define TRACE(...)              printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif


u8 iodm_ver[] = {1,2};

u8 fcc_param[10];
u32 bt_get_xosc_cap(void);
void bt_set_xosc_cap(u32 cap);
u8 param_bt_xosc_read(void);


u8 iodm_cmd_set_bt_name(vh_packet_t *packet)
{
    if(packet->length>32) {
        return IODM_RESULT_FAIL;
    }
    bt_save_new_name((char*)packet->buf, packet->length);
    return IODM_RESULT_OK;
}

u8 iodm_cmd_get_bt_name(u8*tx_buf,u8* tx_len)
{

    if (bt_get_new_name((char *)tx_buf)) {
        *tx_len = strlen((char *)tx_buf);
    } else {
        *tx_len = strlen(xcfg_cb.bt_name);
        memcpy(tx_buf, xcfg_cb.bt_name, *tx_len);
    }

    return IODM_RESULT_OK;
}



u8 iodm_cmd_rsp(vh_packet_t *packet, u8 len, u8 result)
{
    packet->header = 0xAA55;
    packet->distinguish = VHOUSE_DISTINGUISH;
    packet->length = len + 1;
    packet->buf[0] = result;
    vhouse_cmd_ack(packet);
    return 0;
}


void iodm_reveice_data_deal(vh_packet_t *packet)
{
    u8 cmd_rsp_param_len = 0;
    u8 result = IODM_RESULT_OK;
//    print_r(packet,packet->length + 6);
    u8 *tx_buf = (u8*)packet->buf+1;


    switch(packet->cmd){
        case IODM_CMD_DEV_RST:
            TRACE("IODM CMD DEVRST");
			cm_write8(PARAM_RST_FLAG, RST_FLAG_MAGIC_VALUE);
            cm_sync();
            iodm_cmd_rsp(packet, cmd_rsp_param_len, result);
            delay_5ms(10);
        	WDT_RST();
            break;

        case IODM_CMD_SET_BT_ADDR:
            TRACE("IODM CMD SET BT ADDR\n");
            bt_save_qr_addr(packet->buf);
            break;

        case IODM_CMD_GET_BT_ADDR:
            TRACE("IODM CMD GET BT ADDR\n");
            cmd_rsp_param_len = sizeof(xcfg_cb.bt_addr);
            if (!bt_get_qr_addr(tx_buf)) {
                bool bt_get_master_addr(u8 *addr);
                if (!bt_get_master_addr(tx_buf)) {
                    memcpy(tx_buf, xcfg_cb.bt_addr, 6);
                }
            }
            break;

        case IODM_CMD_SET_BT_NAME:
            TRACE("IODM CMD SET BT NAME\n");
            result = iodm_cmd_set_bt_name(packet);
            break;

        case IODM_CMD_GET_BT_NAME:
            TRACE("IODM CMD GET BT NAME\n");
            result = iodm_cmd_get_bt_name(tx_buf,&cmd_rsp_param_len);
            break;

        case IODM_CMD_CBT_TEST_ENTER:
            TRACE("IODM CMD CBT TEST ENTER\n");
            if(func_cb.sta != FUNC_BT_DUT){
                func_cb.sta = FUNC_BT_DUT;
            }
            break;

        case IODM_CMD_CBT_TEST_EXIT:
            TRACE("IODM CMD CBT TEST EXIT\n");
            if (func_cb.sta != FUNC_BT){
                func_cb.sta = FUNC_BT;
                break;
            }
            break;

        case IODM_CMD_FCC_TEST_ENTER:
            TRACE("IODM CMD FCC TEST ENTER\n");
            if (func_cb.sta != FUNC_BT_FCC){
                func_cb.sta = FUNC_BT_FCC;
            }
            break;

        case IODM_CMD_FCC_SET_PARAM:
            TRACE("IODM CMD FCC SET PARAM\n");
            if (func_cb.sta == FUNC_BT_FCC) {
                memcpy(fcc_param, packet->buf, sizeof(fcc_param));
                fcc_param[5] = 7;      //??????power
    //            printf("fcc_param:");
    //            print_r(fcc_param, 0x0a);
                bt_fcc_test_start();
            } else {
                result = IODM_RESULT_FAIL;
            }
            break;

        case IODM_CMD_FCC_TEST_EXIT:
            TRACE("IODM CMD FCC TEST EXIT\n");
            if (func_cb.sta != FUNC_BT){
                func_cb.sta = FUNC_BT;
                break;
            }
            break;

        case IODM_CMD_SET_XOSC_CAP:
            TRACE("IODM CMD SET XOSC CAP\n");
            u8 xtal = packet->buf[0];
            if (xtal < 63) {
                bt_set_xosc_cap(xtal);   //?????? ????????????
            } else {
                result = IODM_RESULT_FAIL;
            }
            break;


        case IODM_CMD_GET_XOSC_CAP:
            TRACE("IODM CMD GET XOSC CAP\n");
            cmd_rsp_param_len = 1;
            u8 cap_param = param_bt_xosc_read();
            if (cap_param == 0xff) {
                tx_buf[0] = xcfg_cb.osci_cap;
            } else {
                cap_param &= ~0x80;
                tx_buf[0] = (cap_param & 0x7) << 3;
                tx_buf[0] |= (cap_param >> 3) & 0x07;
            }
            break;

        case IODM_CMD_GET_INFO:
            TRACE("IODM CMD GET INFO\n");
            cmd_rsp_param_len = 4;
            tx_buf[0] = 1;
            tx_buf[1] = 0;
            tx_buf[2] = 1;
            tx_buf[3] = VH_DATA_LEN;
            break;

        case IODM_CMD_GET_VER_INFO:
            TRACE("IODM CMD GET VER INFO\n");
            cmd_rsp_param_len = 4;
            memcpy(tx_buf, IODM_HARDWARE_VER, 2);
            memcpy(tx_buf+2, IODM_SOFTWARE_VER, 2);
            break;

        case IODM_CMD_PROTOCOL_VER:
            TRACE("IODM CMD PROTOCOL VER\n");
            cmd_rsp_param_len = 2;
            memcpy(tx_buf,iodm_ver,2);
            break;

        case IODM_CMD_CLEAR_PAIR:
            TRACE("IODM CMD CLEAR PAIR\n");
            bt_clr_all_link_info();
            break;

        case IODM_CMD_BT_SET_SCAN:
        case IODM_CMD_MIC_LOOKBACK_ENTER:
        case IODM_CMD_MIC_LOOKBACK_EXIT:
            result = IODM_RESULT_FAIL;
            break;

        default:
            break;
    }
    iodm_cmd_rsp(packet, cmd_rsp_param_len, result);
    TRACE("iodm_reveice_data_deal end\n");
}

uint8_t *bt_rf_get_fcc_param(void)
{
    return fcc_param;
}

void bt_save_qr_addr(u8 *addr)
{
    bt_tws_clr_link_info();
    cm_write8(PARAM_QR_ADDR_VALID, 1);
    cm_write(addr, PARAM_QR_ADDR, 6);
    cm_sync();
}

bool bt_get_qr_addr(u8 *addr)
{
    if (cm_read8(PARAM_QR_ADDR_VALID) == 1) {
        cm_read(addr, PARAM_QR_ADDR, 6);
        return true;
    }
    return false;
}

void bt_save_new_name(char *name,u8 len)
{
    cm_write8(PARAM_BT_NAME_LEN, len);
    cm_write(name, PARAM_BT_NAME, len);
    cm_sync();
}

bool bt_get_new_name(char *name)
{
    u8 len = cm_read8(PARAM_BT_NAME_LEN);
    if(len > 32 || len == 0){
        return false;
    }
    memset(name,0x00,32);  //clear
    cm_read(name, PARAM_BT_NAME, len);
    return true;
}

#endif
