# control-damiao-DM3519-motor
use arduino control damiao DM3519 motor

用 Arduino + USB Host Shield ，
經由 USB‑CAN 轉接器，用 速度模式 控制達妙 DM‑S3519 馬達，
使能 / 失能並以指定 RPM 轉動。


一、通訊與控制模式概念
1. 速度模式 CAN 協議
達妙說明書定義，速度模式下的 CAN 幀是： 
CAN ID ： 0x200 + ID
資料：D0~D3 = v_des（速度給定，float，小端，單位 rad/s）
D4~D7 不使用
也就是：

要讓 CAN ID = 1 的電機以某速度運轉：
發 CAN 幀 ID = 0x200 + 1 = 0x201，D0..D3 放該速度的浮點數(rad/s)，小端序。 

這份程式正是用同樣規則：速度命令時 motor_id = 0x200 + slaveId，D0~D3 放 rpm → rad/s 的浮點數 bytes。
