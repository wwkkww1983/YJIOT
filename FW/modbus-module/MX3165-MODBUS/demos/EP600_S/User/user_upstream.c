/**
******************************************************************************
* @file    user_upstream.c
* @author  herolizhen
* @version V1.0.0
* @date    17 -06-2016
* @brief   This header contains the uart interfaces for user.
operation
******************************************************************************/
#include "mico.h"
#include "fog_v2_include.h"
#include "user_upstream.h"
#include "user_charge.h"
#include "user_device_control.h"

//定义云端保存数据的时间间隔，定时器设置的时间是min
#define STORE_DATA_TIME_INTERVAL   5
//定义定时器定时1min
#define TIMER_1MIN                 (60*1000)
//云端是否保存数据的标志位
volatile unsigned char IsStoreData = 0;
//上传数据时间控制，只有在有筹码和收到开仓门的指令的情况下才5秒钟上传一次数据，其他情况不上传。收到开仓门指令后如果5分钟内没有支付则取消5秒钟上传数据
#define DR_UPLOAD_DATA_TIME_INTERVAL  7
volatile unsigned char upload_data_time_control = 0;
volatile unsigned char dr_upload_data_time_control = 0;
//第一次上传的数据要保存
volatile unsigned char Is_First_Upload_Sucess = 0;
//定时器
mico_timer_t store_data_timer;


int upload_try_times = 0;

extern volatile unsigned char is_http_get_dr_cmd;

extern user_charge_t user_charge;
extern device_status_t device_status;

extern unsigned char recv_0x41_data[DATA_LENGTH_0x41] ;
extern unsigned char recv_0x45_data[DATA_LENGTH_0x45] ;
extern bool is_recv_0x41_data ;
extern bool is_recv_0x45_data ;
extern OSStatus upload_dev_data ( unsigned char *recv_0x41_buf, unsigned char *recv_0x45_buf, mico_Context_t *mico_context ) ;

//定时器回调函数 1mim定时器
void storedata_timer_callback(void *arg)
{
    static uint32_t timer_interrupt_times = 0;
    timer_interrupt_times += 1;
    if(is_http_get_dr_cmd == 1)
    {
        dr_upload_data_time_control += 1;
        if(dr_upload_data_time_control >= DR_UPLOAD_DATA_TIME_INTERVAL)
        {
            is_http_get_dr_cmd = 0;
            dr_upload_data_time_control = 0;
        }
    }
    
    
    //user_modbus_log("come in storedata_timer_callback %d times",timer_interrupt_times);
    if(timer_interrupt_times >= STORE_DATA_TIME_INTERVAL)
    {
        IsStoreData = 1;       
        //user_modbus_log("timer %d min come",timer_interrupt_times);
        timer_interrupt_times = 0;
        
    }   
    mico_rtos_start_timer(&store_data_timer);
}

void user_upstream_thread ( mico_thread_arg_t arg ) {
  
  //unsigned char temp1,temp2 = 0;
  
  unsigned char is_upload_data = 0;

  OSStatus err = kUnknownErr;
  mico_Context_t *mico_context = ( mico_Context_t * ) arg;
  require ( mico_context, exit );
  
  mico_rtos_init_timer( &store_data_timer, TIMER_1MIN, storedata_timer_callback, NULL );
  mico_rtos_start_timer(&store_data_timer);
  
  while ( 1 ) {
    f_0x45_cmd();
    mico_thread_sleep ( 1 );
    f_0x41_04_cmd() ;
    mico_thread_sleep ( 2 );
    //temp1 = is_http_get_dr_cmd;
    //temp2 = dr_upload_data_time_control;
    //user_upstream_log("is_http_get_dr_cmd:%d,upload_data_time_control:%d",temp1,temp2);
    
    if(user_charge.lefttime.IsChargeType == 1 && user_charge.lefttime.left_time > 0)
    {
        is_upload_data = 1;
    }
    if(IsStoreData == 1 || is_http_get_dr_cmd == 1 || Is_First_Upload_Sucess == 0)
    {
        is_upload_data = 1;
    }
    
    if(is_upload_data == 1)
    {      
      if(dr_upload_data_time_control >= DR_UPLOAD_DATA_TIME_INTERVAL)
      {
        is_http_get_dr_cmd = 0;
        dr_upload_data_time_control = 0;
      }
      
      upload_dev_data ( recv_0x41_data, recv_0x45_data, mico_context );
      
      is_upload_data = 0;
    }
    
    //upload_dev_data ( recv_0x41_data, recv_0x45_data, mico_context );
    
    mico_thread_sleep ( 2 );
  }
  
exit:
  if ( kNoErr != err ) {
    user_upstream_log ( "ERROR: user_upstream_thread exit with err=%d", err );
  }
  mico_rtos_delete_thread ( NULL );
}


union Conv {
  float f;
  unsigned char  b[4];
} cFloat;



OSStatus upload_dev_data ( unsigned char *recv_0x41_buf, unsigned char *recv_0x45_buf, mico_Context_t *mico_context ) {
  
  OSStatus err = kNoErr;
  
  
  char * sensor_type = NULL;
  char * ret_sensor_type = NULL;
  
  int frame_size_0x41  = ( int ) recv_0x41_buf[0];
  json_object *send_json_object = NULL;
  const char *upload_data = NULL;
  
  if ( is_recv_0x41_data == false || is_recv_0x45_data == false ) {
    user_upstream_log ( "No data to upload!" );
    return 0;
  }
  
  send_json_object = json_object_new_object();
  require ( send_json_object, exit );
  
  int j ;
  j = 0 ;
  for ( int i = 5; i < frame_size_0x41 - 6; i += 5 ) {
  

    cFloat.b[3] = recv_0x41_buf[i + 1];
    cFloat.b[2] = recv_0x41_buf[i + 2];
    cFloat.b[1] = recv_0x41_buf[i + 3];
    cFloat.b[0] = recv_0x41_buf[i + 4];
    
    switch ( j ) {
    case 0:
      sensor_type = "P";   //PM2.5
      break;
    case 1:
      sensor_type = "Z";    //PM1.0
      break;
    case 2:
      sensor_type = "O";   //PM10
      break;
    case 3:
      sensor_type = "J";   //HCHO
      break;   
    default:
      break;
    }
    
    
    

    ret_sensor_type = malloc ( sizeof ( sensor_type ) );
    memset ( ret_sensor_type, '\0', sizeof ( sensor_type ) );
    memcpy ( ret_sensor_type, sensor_type, sizeof ( sensor_type ) );
    
    if ( j < 3 ) {
      //user_upstream_log ( "PM2.5 value:[%f]", cFloat.f );
      json_object_object_add ( send_json_object, ret_sensor_type, json_object_new_int ( ( int )cFloat.f ) );     
    } else if(j == 3){
      json_object_object_add ( send_json_object, ret_sensor_type, json_object_new_double ( ( ( double ) ( ( int ) ( cFloat.f * 1000 ) ) ) / 1000.0 ) );
    }
    
    if(ret_sensor_type != NULL)
    {
      free ( ret_sensor_type );
      ret_sensor_type = NULL;
    }
    
    j++;

  }
  
  //int sys_mode ;   //系统模式，分为运行和待机
  device_status.sys_mode = ( int ) recv_0x45_buf[5];
  //int SM_status ;   //分为手动和自动模式
  device_status.SM_status = ( int ) recv_0x45_buf[6];
  //int FA_status ;
  device_status.FA_status = ( int ) recv_0x45_buf[7];
  //int UV_status ;
  device_status.UV_status = ( int ) recv_0x45_buf[8];  
  //int NI_status ;
  device_status.NI_status = ( int ) recv_0x45_buf[9];

  json_object_object_add ( send_json_object, "SMD", json_object_new_int ( device_status.sys_mode ) );
  json_object_object_add ( send_json_object, "SM", json_object_new_int ( device_status.SM_status ) );
  json_object_object_add ( send_json_object, "FA", json_object_new_int ( device_status.FA_status ) );
  json_object_object_add ( send_json_object, "UV", json_object_new_int ( device_status.UV_status ) );
  json_object_object_add ( send_json_object, "NI", json_object_new_int ( device_status.NI_status ) );
  
  
  //计费时间
  json_object_object_add ( send_json_object, "TYPE", json_object_new_int ( (int)user_charge.lefttime.IsChargeType ) );
  json_object_object_add ( send_json_object, "CHIPS", json_object_new_int ( (int)user_charge.lefttime.left_time ) );
  

  
  upload_data = json_object_to_json_string ( send_json_object );
  require ( upload_data, exit );
  // check fogcloud connect status
  
  if(fog_v2_is_https_connect() == true && get_wifi_status() == true){
    user_modbus_log("upload_data:%s", upload_data);
    
    if(IsStoreData == 0)
    {
        if(Is_First_Upload_Sucess == 0)
        {
            //user_modbus_log("unstream way 1");
            err = fog_v2_device_send_event(upload_data, 1);   
        }
        else
        {
            //user_modbus_log("unstream way 2");
            err = fog_v2_device_send_event(upload_data, 0);
        }
        
    }
    else if(IsStoreData == 1)
    {
        //user_modbus_log("unstream way 3");   
        err = fog_v2_device_send_event(upload_data, 1);   
        IsStoreData = 0;
    }
    
    if(kNoErr != err){
      user_modbus_log("ERROR: Upload f_0x41_04_data failed, Err=%d", err);
      upload_try_times += 1 ;
      goto exit;
    }else{
      if(Is_First_Upload_Sucess == 0)
      {
          Is_First_Upload_Sucess = 1;
      }
      upload_try_times = 0 ;
    }
  }  
  else {
      user_modbus_log("ERROR: Upload f_0x41_04_data failed, because WIFI or http disconnect!");
      upload_try_times += 1;
  }
  
 exit:
   
  if(ret_sensor_type != NULL)
  {
    free ( ret_sensor_type );
    ret_sensor_type = NULL;
  }
   
  // free json object memory
  if(NULL != send_json_object){
    json_object_put(send_json_object);
    send_json_object = NULL;
  }
  //rebooting  for mutl upload failed
  if(upload_try_times > UPLOAD_TRY_COUNT){
    user_modbus_log("ERROR: Mico reboot because multiple upload failed!");
    MicoSystemReboot();
  }
  return err;
}


OSStatus upload_door_data (int door_data,mico_Context_t *mico_context )
{
  OSStatus err = kNoErr;
  
  char * ret_key = "DR";
  int door_value = 0;
  unsigned char upload_try_times = 0;
  
  json_object *send_json_object = NULL;
  const char *upload_data = NULL;
  
  send_json_object = json_object_new_object();
  require ( send_json_object, exit );
  
  door_value = door_data;

  json_object_object_add ( send_json_object, ret_key, json_object_new_int ( door_value ) );
  

  
  upload_data = json_object_to_json_string ( send_json_object );
  require ( upload_data, exit );
  // check fogcloud connect status
  
  while(upload_try_times < 6)
  {
    if(fog_v2_is_https_connect() == true && get_wifi_status() == true){
      user_modbus_log("upload_data:%s", upload_data);
    
      err = fog_v2_device_send_event(upload_data, 0);    
    
      if(kNoErr != err){
        user_modbus_log("ERROR: Upload door data failed %d times, Err=%d",(upload_try_times + 1), err);
        goto try_again;
      }else{
        goto exit ;
      }
    }  
    else {
      user_modbus_log("WIFI info  not config or missing,try %d times!",(upload_try_times + 1));
      goto try_again;
    }
 try_again:
   upload_try_times += 1;
   mico_thread_msleep(500);
    
  }

  
 exit:
   
  // free json object memory
  if(NULL != send_json_object){
    json_object_put(send_json_object);
    send_json_object = NULL;
  }
  return err;

}






