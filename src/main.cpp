#include <Arduino.h>
#include <Wire.h>
#include <SCServo.h>

#define I2C_DEV_ADDR 0x66

#define TX_PIN TX
#define RX_PIN RX

#define LEG_ROUND 2048

#define LEG_1_FORWORD 1058 // 1013 //(´∀｀∩)↑age↑
#define LEG_2_FORWORD 3172
#define LEG_3_FORWORD 3062 // 3084 ////3037
#define LEG_4_FORWORD 991  // 990 //948 // 1090

#define LEG_1_BACK 1058
#define LEG_2_BACK 3172 ////3114
#define LEG_3_BACK 3062
#define LEG_4_BACK 991

#define LEG_1_RIGHT 3050
#define LEG_2_RIGHT 1020
#define LEG_3_RIGHT 1090
#define LEG_4_RIGHT 3040

int DATA[8] = {0};

SMS_STS st;

void servo_setup();
void servo_setup_sync();
void set_direction(String servo_direction);
void set_direction_x2(String servo_direction);
void set_direction_sync(String servo_direction);

void onReceive(int len)
{
    Serial.print("Received len: ");
    Serial.println(len);
    for (int i = 0; i < len && i < 8; i++)
    {
        DATA[i] = Wire.read();
        Serial.print("DATA[");
        Serial.print(i);
        Serial.print("]: ");
        Serial.println(DATA[i], HEX);
    }

    switch (DATA[0])
    {
    case 0x01:
        set_direction_sync("front");
        break;

    case 0x02:
        set_direction_sync("back");
        break;

    case 0x03:
        set_direction_sync("left");
        break;

    case 0x04:
        set_direction_sync("right");
        break;

    case 0x05:
        set_direction_sync("round");
        break;

    default:
        break;
    }
}

void servo_setup()
{

    Serial1.begin(1000000, SERIAL_8N1, RX_PIN, TX_PIN);
    st.pSerial = &Serial1;
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (int i = 1; i <= 8; i++)
    {

        // st.unLockEprom(i); // EEPROMのロック解除
        // st.writeByte(i, SMS_STS_MODE, 0);
        // st.LockEprom(i); // EEPROMを再ロック
        // st.EnableTorque(i, 1);

        delay(1);
    }
}

void servo_setup_sync()
{
    Serial1.begin(1000000, SERIAL_8N1, RX_PIN, TX_PIN);
    st.pSerial = &Serial1;
}

void set_direction(String servo_direction)
{

    delay(50);

    if (servo_direction == "front")
    {
        st.RegWritePosEx(1, LEG_1_FORWORD, 3400);
        st.RegWritePosEx(2, LEG_2_FORWORD, 3400);
        st.RegWritePosEx(3, LEG_3_FORWORD, 3400);
        st.RegWritePosEx(4, LEG_4_FORWORD, 3400);
    }
    else if (servo_direction == "back")
    {
        st.RegWritePosEx(1, LEG_1_BACK, 3400);
        st.RegWritePosEx(2, LEG_2_BACK, 3400);
        st.RegWritePosEx(3, LEG_3_BACK, 3400);
        st.RegWritePosEx(4, LEG_4_BACK, 3400);
    }
    else if (servo_direction == "left" || servo_direction == "right")
    {
        st.RegWritePosEx(1, LEG_1_RIGHT, 3400);
        st.RegWritePosEx(2, LEG_2_RIGHT, 3400);
        st.RegWritePosEx(3, LEG_3_RIGHT, 3400);
        st.RegWritePosEx(4, LEG_4_RIGHT, 3400);
    }
    else if (servo_direction == "round")
    {
        st.RegWritePosEx(1, LEG_ROUND, 3400);
        st.RegWritePosEx(2, LEG_ROUND, 3400);
        st.RegWritePosEx(3, LEG_ROUND, 3400);
        st.RegWritePosEx(4, LEG_ROUND, 3400);
    }
    else
    {
        Serial.printf("error!");
    }

    delay(200);

    st.RegWriteAction();
}

void set_direction_x2(String servo_direction)
{

    delay(50);

    if (servo_direction == "front")
    {
        st.RegWritePosEx(1, LEG_1_FORWORD, 3400);
        st.RegWritePosEx(4, LEG_4_FORWORD, 3400);
        st.RegWriteAction();
        vTaskDelay(pdMS_TO_TICKS(50));
        st.RegWritePosEx(2, LEG_2_FORWORD, 3400);
        st.RegWritePosEx(3, LEG_3_FORWORD, 3400);
    }
    else if (servo_direction == "back")
    {
        st.RegWritePosEx(1, LEG_1_BACK, 3400);
        st.RegWritePosEx(4, LEG_4_BACK, 3400);
        st.RegWriteAction();
        vTaskDelay(pdMS_TO_TICKS(50));
        st.RegWritePosEx(2, LEG_2_BACK, 3400);
        st.RegWritePosEx(3, LEG_3_BACK, 3400);
    }
    else if (servo_direction == "left" || servo_direction == "right")
    {
        st.RegWritePosEx(1, LEG_1_RIGHT, 3400);
        st.RegWritePosEx(4, LEG_4_RIGHT, 3400);
        st.RegWriteAction();
        vTaskDelay(pdMS_TO_TICKS(50));
        st.RegWritePosEx(2, LEG_2_RIGHT, 3400);
        st.RegWritePosEx(3, LEG_3_RIGHT, 3400);
    }
    else if (servo_direction == "round")
    {
        st.RegWritePosEx(1, LEG_ROUND, 3400);
        st.RegWritePosEx(4, LEG_ROUND, 3400);
        st.RegWriteAction();
        vTaskDelay(pdMS_TO_TICKS(50));
        st.RegWritePosEx(2, LEG_ROUND, 3400);
        st.RegWritePosEx(3, LEG_ROUND, 3400);
    }
    else
    {
        Serial.printf("error!");
    }

    delay(200);

    st.RegWriteAction();
}

void set_direction_sync(String servo_direction)
{

    delay(50);

    u8 ID[4] = {1, 2, 3, 4};
    u16 speed[4] = {3400, 3400, 3400, 3400};
    u8 acc[4] = {50, 50, 50, 50};

    if (servo_direction == "front")
    {
        s16 POSITION[4] = {LEG_1_FORWORD, LEG_2_FORWORD, LEG_3_FORWORD, LEG_4_FORWORD};

        st.SyncWritePosEx(ID, sizeof(ID), POSITION, speed, acc);
    }
    else if (servo_direction == "back")
    {
        s16 POSITION[4] = {LEG_1_BACK, LEG_2_BACK, LEG_3_BACK, LEG_4_BACK};

        st.SyncWritePosEx(ID, sizeof(ID), POSITION, speed, acc);
    }
    else if (servo_direction == "left" || servo_direction == "right")
    {
        s16 POSITION[4] = {LEG_1_RIGHT, LEG_2_RIGHT, LEG_3_RIGHT, LEG_1_RIGHT};

        st.SyncWritePosEx(ID, sizeof(ID), POSITION, speed, acc);
    }
    else if (servo_direction == "round")
    {
        s16 POSITION[4] = {LEG_ROUND, LEG_ROUND, LEG_ROUND, LEG_ROUND};

        st.SyncWritePosEx(ID, sizeof(ID), POSITION, speed, acc);
    }
    else
    {
        Serial.printf("error!");
    }

    delay(200);
}

void setup()
{

    Serial.begin(115200);
    delay(1000);

    // XIAO ESP32-C6のデフォルトピン(D4=SDA, D5=SCL)とアドレスを指定
    Wire.begin(I2C_DEV_ADDR, D4, D5, 100000);
    Wire.onReceive(onReceive);
    Serial.println("Slave Ready on D4/D5");

    servo_setup_sync();
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1));
}
