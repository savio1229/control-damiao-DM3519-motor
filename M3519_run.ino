#include <Usb.h>
#include <cdcacm.h>
#include <usbhub.h>

// ============ USB Host / CDC 初始化 ============
USB Usb;
class ACMAsyncOper : public CDCAsyncOper {
public:
  uint8_t OnInit(ACM *pacm) override { return 0; }
};
ACMAsyncOper AsyncOper;
ACM Acm(&Usb, &AsyncOper);
bool deviceReady = false;

// ============ 馬達 / 協議參數 ============
// 馬達的 CAN SlaveID
const uint16_t MOTOR_SLAVE_ID = 0x01;
// 速度模式下的 motor_id = 0x200 + SlaveID（對應 control_Vel）
const uint16_t MOTOR_ID_VEL_BASE = 0x200;
// 串口 921600, 8N1
const uint32_t UART_BAUD = 921600;
// RPM 上限（防呆）
const float RPM_MAX = 395.0f;

// ============ send_data_frame 模板（照 DM_CAN.MotorControl）===========
uint8_t send_data_frame_template[30] = {
  0x55, 0xAA, 0x1e, 0x03, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ============ 工具：RPM→rad/s、float→4bytes 小端 ============
float rpm_to_rad(float rpm) {
  return rpm * 0.1047197551f;   // m3519_run.py 同樣係數
}

void f32_to_bytes_le(float f, uint8_t out[4]) {
  union { float f; uint32_t u; } v;
  v.f = f;
  out[0] = (uint8_t)(v.u & 0xFF);
  out[1] = (uint8_t)((v.u >> 8) & 0xFF);
  out[2] = (uint8_t)((v.u >> 16) & 0xFF);
  out[3] = (uint8_t)((v.u >> 24) & 0xFF);
}

// ============ USB 串口 I/O 封裝 ============
void usbSerialWrite(const uint8_t *buf, uint16_t len) {
  if (!deviceReady) return;
  Acm.SndData(len, const_cast<uint8_t*>(buf));
}

// ============ 等價 Python 的 __send_data(motor_id, data) ============
void sendFrameToMotor(uint16_t motor_id, const uint8_t data[8]) {
  if (!deviceReady) return;

  uint8_t frame[30];
  for (uint8_t i = 0; i < 30; i++) {
    frame[i] = send_data_frame_template[i];
  }

  // send_data_frame[13/14] = motor_id
  frame[13] = (uint8_t)(motor_id & 0xFF);
  frame[14] = (uint8_t)((motor_id >> 8) & 0xFF);

  // send_data_frame[21:29] = data
  for (uint8_t i = 0; i < 8; i++) {
    frame[21 + i] = data[i];
  }

  usbSerialWrite(frame, 30);
}

// ============ 速度命令：等價 control_Vel + motor_run ============
void buildVelData(float rpm, uint8_t data[8]) {
  if (rpm >  RPM_MAX) rpm  =  RPM_MAX;
  if (rpm < -RPM_MAX) rpm  = -RPM_MAX;

  float rad_s = rpm_to_rad(rpm);
  uint8_t v[4];
  f32_to_bytes_le(rad_s, v);

  data[0] = v[0];
  data[1] = v[1];
  data[2] = v[2];
  data[3] = v[3];
  data[4] = 0;
  data[5] = 0;
  data[6] = 0;
  data[7] = 0;
}

// 傳入「電機 SlaveID」和「RPM」
void sendVelCommand(uint16_t slaveId, float rpm) {
  Usb.Task();               // 維持 USB Host 狀態
  if (!deviceReady) return;

  uint16_t motor_id = MOTOR_ID_VEL_BASE + slaveId;   // 0x200 + SlaveID

  uint8_t data[8];
  buildVelData(rpm, data);
  sendFrameToMotor(motor_id, data);
}

// ============ 使能與失能（等價 enable / disable） ============
void send_Enable(uint16_t slaveId) {
  Usb.Task();
  if (!deviceReady) return;
  uint8_t data[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC};
  sendFrameToMotor(slaveId, data);
}

void send_disable(uint16_t slaveId) {
  Usb.Task();
  if (!deviceReady) return;
  uint8_t data[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFD};
  sendFrameToMotor(slaveId, data);
}

// ============ 馬達與 USB 的初始化打包成一個函數 ============
void initMotor() {
  // 初始化 USB Host
  if (Usb.Init() == -1) {
    while (1) {}   // 失敗就卡死
  }

  // 等 USB‑CAN 枚舉完成（CDC ready）
  while (!Acm.isReady()) {
    Usb.Task();
  }
  deviceReady = true;

  // 設定虛擬串口 921600, 8N1
  LINE_CODING lc;
  lc.dwDTERate = UART_BAUD;
  lc.bCharFormat = 0;
  lc.bParityType = 0;
  lc.bDataBits   = 8;
  Acm.SetLineCoding(&lc);

  delay(100);                // 給 USB‑CAN / 馬達一點時間穩定
}

// ============ setup / loop ============
void setup() {
  Serial.begin(115200);
  initMotor();   // 所有馬達 setup 都包在這一行
  send_Enable(MOTOR_SLAVE_ID);      // 上電後一次性使能
}

void loop() {
  send_Enable(MOTOR_SLAVE_ID);
  sendVelCommand(0x01, 10.0f);
  delay(1000);
  send_disable(MOTOR_SLAVE_ID);
  delay(1000);
}
