#include "include.h"
#include "bsp_ble.h"
#if LE_APP_EN
#include "ab_link/ab_link.h"
#endif

#if LE_EN && !LE_DUEROS_EN

const bool cfg_ble_security_en = LE_PAIR_EN;

//可重定义该函数修改ble地址
//void ble_get_local_bd_addr(u8 *addr)
//{
//    memset(addr, 0xaa, 6);
//}

//可重定义该函数修改ble地址类型
//u8 ble_get_local_addr_mode(void)
//{
//    return 1;   //0:public 1:random no resolvable 2:random resolvable
//}

void ble_disconnect_callback(void)
{
    printf("--->ble_disconnect_callback\n");

#if FOT_EN
    fot_ble_disconnect_callback();
#endif
}

void ble_connect_callback(void)
{
    printf("--->ble_connect_callback\n");
#if FOT_EN
    fot_ble_connect_callback();
#endif
}

uint ble_get_bat_level(void)
{
    return bsp_get_bat_level();
}

void ble_init_att(void)
{
    ble_app_init();
}
#endif // LE_EN
