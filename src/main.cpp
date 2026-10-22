#include <Arduino.h>
#include <Wire.h>
#include <SCServo.h>

#define I2C_DEV_ADDR 0x66

#define TX_PIN TX
#define RX_PIN RX

// ---------------- 状態 ----------------
#define STATE_IDLE 0x00
#define STATE_BUSY 0x01

#define RESULT_OK 0x00
#define RESULT_ERROR 0x01

// ---------------- 足サーボ目標位置 ----------------
#define LEG_ROUND 2048

#define LEG_1_FORWORD 1058
#define LEG_2_FORWORD 3172
#define LEG_3_FORWORD 3062
#define LEG_4_FORWORD 991

#define LEG_1_BACK 1058
#define LEG_2_BACK 3172
#define LEG_3_BACK 3062
#define LEG_4_BACK 991

#define LEG_1_RIGHT 3050
#define LEG_2_RIGHT 1020
#define LEG_3_RIGHT 1090
#define LEG_4_RIGHT 3040

#define POS_TOLERANCE 20       // 到達とみなす許容誤差
#define MOVE_TIMEOUT_MS 8000   // 1回の向き変更の完了待ち上限

SMS_STS st;

// ---------------- マスターとの共有変数 ----------------
// onReceive/onRequest は I2C 側のコンテキストで呼ばれるため、
// そこでは変数を書き換えるだけにして、サーボ制御は loop() で行う
volatile uint8_t g_state = STATE_BUSY; // 起動中はBUSY
volatile uint8_t g_done_seq = 0;       // 最後に完了したコマンドの seq
volatile uint8_t g_result = RESULT_OK;

volatile uint8_t g_pending_cmd = 0;
volatile uint8_t g_pending_seq = 0;
volatile bool g_pending = false;

// ---------------- I2C コールバック ----------------
// 受信フォーマット: DATA[0]=コマンド, DATA[1]=seq
void onReceive(int len)
{
    uint8_t buf[8] = {0};
    int n = 0;
    while (Wire.available())
    {
        uint8_t b = Wire.read();
        if (n < 8)
            buf[n++] = b;
    }
    if (n < 2)
        return;

    // 処理中に来たコマンドは無視(マスターは完了を待ってから次を送る前提)
    if (g_state == STATE_BUSY || g_pending)
        return;

    g_pending_cmd = buf[0];
    g_pending_seq = buf[1];
    g_state = STATE_BUSY; // 先にBUSYにしておく
    g_pending = true;     // 最後にフラグを立てる
}

// マスターの requestFrom(addr, 3) に対して [state, done_seq, result] を返す
void onRequest()
{
    Wire.write((uint8_t)g_state);
    Wire.write((uint8_t)g_done_seq);
    Wire.write((uint8_t)g_result);
}

// ---------------- サーボ ----------------
void servo_setup_sync()
{
    Serial1.begin(1000000, SERIAL_8N1, RX_PIN, TX_PIN);
    st.pSerial = &Serial1;
}

// 全サーボが目標位置の許容誤差内に入るまで待つ
bool waitForServo(const u8 *ids, const s16 *targets, int n,
                  uint32_t timeout = MOVE_TIMEOUT_MS, int tol = POS_TOLERANCE)
{
    uint32_t start = millis();
    delay(30); // 指令直後は位置が変わり始めていないので少し待つ

    while (millis() - start < timeout)
    {
        bool all_completed = true;
        for (int i = 0; i < n; i++)
        {
            int pos = st.ReadPos(ids[i]);
            if (pos == -1 || abs(pos - (int)targets[i]) > tol)
            {
                all_completed = false;
                break;
            }
        }
        if (all_completed)
            return true;

        vTaskDelay(pdMS_TO_TICKS(15));
    }

    Serial.println("Warning: Servo Motion Timeout (legs)");
    return false;
}

// コマンド → 目標位置テーブル
bool get_targets(uint8_t cmd, s16 out[4])
{
    switch (cmd)
    {
    case 0x01: // front
        out[0] = LEG_1_FORWORD;
        out[1] = LEG_2_FORWORD;
        out[2] = LEG_3_FORWORD;
        out[3] = LEG_4_FORWORD;
        return true;
    case 0x02: // back
        out[0] = LEG_1_BACK;
        out[1] = LEG_2_BACK;
        out[2] = LEG_3_BACK;
        out[3] = LEG_4_BACK;
        return true;
    case 0x03: // left
    case 0x04: // right
        out[0] = LEG_1_RIGHT;
        out[1] = LEG_2_RIGHT;
        out[2] = LEG_3_RIGHT;
        out[3] = LEG_4_RIGHT; // 元コードは LEG_1_RIGHT になっていたので修正
        return true;
    case 0x05: // round
        out[0] = LEG_ROUND;
        out[1] = LEG_ROUND;
        out[2] = LEG_ROUND;
        out[3] = LEG_ROUND;
        return true;
    default:
        return false;
    }
}

// 足を指定の向きにし、到達まで待つ
bool set_direction_sync(uint8_t cmd)
{
    u8 ID[4] = {1, 2, 3, 4};
    u16 speed[4] = {3400, 3400, 3400, 3400};
    u8 acc[4] = {50, 50, 50, 50};
    s16 POSITION[4];

    if (!get_targets(cmd, POSITION))
    {
        Serial.printf("error! unknown cmd: 0x%02X\n", cmd);
        return false;
    }

    st.SyncWritePosEx(ID, sizeof(ID), POSITION, speed, acc);
    return waitForServo(ID, POSITION, 4);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // XIAO ESP32 のデフォルトピン(D4=SDA, D5=SCL)とアドレスを指定
    Wire.begin(I2C_DEV_ADDR, D4, D5, 100000);
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest); // 元コードでは未登録だった
    Serial.println("Slave(LEG) Ready on D4/D5");

    servo_setup_sync();

    g_result = RESULT_OK;
    g_state = STATE_IDLE;
}

void loop()
{
    if (g_pending)
    {
        uint8_t cmd = g_pending_cmd;
        uint8_t seq = g_pending_seq;
        g_pending = false;

        Serial.printf("cmd=0x%02X seq=%d\n", cmd, seq);

        bool ok = set_direction_sync(cmd);

        // 結果 → seq → state の順に更新(state が IDLE になった時点で全て確定)
        g_result = ok ? RESULT_OK : RESULT_ERROR;
        g_done_seq = seq;
        g_state = STATE_IDLE;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
}
